// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxLocation.h"
#include "RoxFormats/RoxTextParser.h"
#include "RoxFormats/RoxStringConvert.h"
#include "RoxRender/RoxRender.h" // Assuming render.h is needed
#include "RoxMemory/RoxInvalidObject.h"
#include <cstring> // Replaced <string.h> with <cstring>

namespace RoxScene
{

    // Initialize the active camera proxy
    namespace { RoxLocationProxy activeCamera = RoxLocationProxy(RoxLocation()); }

    bool RoxLocation::loadText(SharedLocation& res, ResourceData& data, const char* name)
    {
        RoxFormats::TextParser parser;
        if (!parser.loadFromData(reinterpret_cast<char*>(data.getData()), data.getSize()))
            return false;

        RTags localTags;
        std::string localMeshPath;
        for (int i = 0; i < parser.getSectionsCount(); ++i)
        {
            const char* type = parser.getSectionType(i);
            if (std::strcmp(type, "@object") == 0)
            {
                SharedLocation::LocationMesh m;
                m.tg = localTags;
                for (int j = 0; j < parser.getSubsectionsCount(i); ++j)
                {
                    const char* subType = parser.getSubsectionType(i, j);
                    if (std::strcmp(subType, "mesh") == 0)
                        m.name = localMeshPath + parser.getSubsectionValue(i, j);
                    else if (std::strcmp(subType, "tags") == 0)
                        m.tg.addTag(RTags(parser.getSubsectionValue(i, j)));
                    else if (std::strcmp(subType, "pos") == 0)
                    {
                        const RoxMath::Vector4 v = RoxFormats::vec4FromString(parser.getSubsectionValue(i, j));
                        m.tr.setPos(v.xyz());
                    }
                    else if (std::strcmp(subType, "rot") == 0)
                    {
                        const RoxMath::Vector4 v = RoxFormats::vec4FromString(parser.getSubsectionValue(i, j));
                        m.tr.setRot(v.x, v.y, v.z);
                    }
                    else if (std::strcmp(subType, "scale") == 0)
                    {
                        const RoxMath::Vector4 v = RoxFormats::vec4FromString(parser.getSubsectionValue(i, j));
                        m.tr.setScale(v.x, v.y, v.z);
                    }
                }

                res.meshes.push_back(m);
            }
            else if (std::strcmp(type, "@material_param") == 0)
            {
                RoxMath::Vector4 v = parser.getSectionValueVec4(i);
                const char* opt = parser.getSectionOption(i);
                if (std::strcmp(opt, "normalize") == 0)
                    v.normalize();

                res.materialParams.emplace_back(parser.getSectionName(i), v);
            }
            else if (std::strcmp(type, "@tags") == 0)
                localTags = parser.getSectionValue(i);
            else if (std::strcmp(type, "@mesh_folder") == 0)
                localMeshPath.assign(parser.getSectionValue(i));
        }

        return true;
    }

    bool RoxLocation::load(const char* name)
    {
        defaultLoadFunction(loadText);
        unload();

        if (!RoxSceneShared<SharedLocation>::load(name))
            return false;

        if (!mShared.isValid())
            return false;

        mMaterialParams.resize(mShared->materialParams.size());
        for (size_t j = 0; j < mShared->materialParams.size(); ++j)
        {
            mMaterialParams[j].first = mShared->materialParams[j].first;
            mMaterialParams[j].second = RoxMaterial::ParamProxy(mShared->materialParams[j].second);
        }

        for (size_t i = 0; i < mShared->meshes.size(); ++i)
        {
            const SharedLocation::LocationMesh& m = mShared->meshes[i];
            addMesh(m.name.c_str(), m.tg, m.tr);
        }

        return true;
    }

    void RoxLocation::unload()
    {
        mMeshes.clear();
        mMaterialParams.clear();
        RoxSceneShared<SharedLocation>::unload();
    }

    int RoxLocation::addMesh(const char* meshName, const RTags& tg, const Transform& tr)
    {
        const int meshIdx = mMeshes.getCount();

        std::vector<const char*> tp(tg.getCount());
        for (int i = 0; i < tg.getCount(); ++i) tp[i] = tg.getTag(i);
        LocationMeshInternal& lm = mMeshes.add(tp.empty() ? nullptr : tp.data(), tg.getCount());
        if (meshName)
            lm.m.load(meshName);
        lm.m.setTransform(tr);
        lm.visible = true;
        lm.needApply = true;
        mNeedApply = true;

        return meshIdx;
    }

    RoxMesh& RoxLocation::modifyMesh(int idx, bool needApply)
    {
        LocationMeshInternal& m = mMeshes.get(idx);
        if (needApply)
            mNeedApply = m.needApply = true;
        return m.m;
    }

    RoxMesh& RoxLocation::modifyMesh(const char* tag, int idx, bool needApply)
    {
        return modifyMesh(mMeshes.getIdx(tag, idx), needApply);
    }

    void RoxLocation::update(int dt)
    {
        if (mNeedApply)
        {
            for (int i = 0; i < mMeshes.getCount(); ++i)
            {
                LocationMeshInternal& lm = mMeshes.get(i);

                if (!lm.needApply)
                    continue;

                for (int j = 0; j < lm.m.getGroupsCount(); ++j)
                {
                    for (size_t k = 0; k < mMaterialParams.size(); ++k)
                    {
                        const int paramIdx = lm.m.getMaterial(j).getParamIdx(mMaterialParams[k].first.c_str());
                        if (paramIdx < 0)
                            continue;

                        lm.m.modifyMaterial(j).setParam(paramIdx, mMaterialParams[k].second);
                    }
                }

                lm.needApply = false;
            }

            mNeedApply = false;
        }

        for (int i = 0; i < mMeshes.getCount(); ++i)
            mMeshes.get(i).m.update(dt);
    }

    void RoxLocation::draw(const char* pass, const RTags& t) const
    {
        if (t.getCount() <= 1)
        {
            for (int i = 0; i < mMeshes.getCount(t.getTag(0)); ++i)
            {
                const LocationMeshInternal& lm = mMeshes.get(i);
                if (!lm.visible)
                    continue;

                lm.m.draw(pass);
            }

            return;
        }

        mDrawCache.clear();
        mDrawCache.resize(mMeshes.getCount(), false);
        for (int i = 0; i < t.getCount(); ++i)
        {
            const char* tag = t.getTag(i);
            for (int j = 0; j < mMeshes.getCount(tag); ++j)
            {
                const int meshIdx = mMeshes.getIdx(tag, j);
                if (mDrawCache[meshIdx])
                    continue;

                const LocationMeshInternal& lm = mMeshes.get(meshIdx);
                if (!lm.visible)
                    continue;

                lm.m.draw(pass);
                mDrawCache[meshIdx] = true;
            }
        }
    }

    const char* RoxLocation::getMaterialParamName(int idx) const
    {
        if (idx < 0 || idx >= getMaterialParamsCount())
            return nullptr;

        return mMaterialParams[idx].first.c_str();
    }

    const RoxMaterial::ParamProxy& RoxLocation::getMaterialParam(int idx) const
    {
        if (idx < 0 || idx >= getMaterialParamsCount())
            return RoxMemory::invalidObject<RoxMaterial::ParamProxy>();

        return mMaterialParams[idx].second;
    }

    void RoxLocation::setMaterialParam(int idx, const RoxMaterial::Param& p)
    {
        if (idx < 0 || idx >= getMaterialParamsCount())
            return;

        mMaterialParams[idx].second.set(p);
    }

    // External function implementations
    void setCamera(const RoxLocationProxy& cam)
    {
        activeCamera = cam;
        if (cam.isValid())
            RoxRender::setProjectionMatrix(cam->getProjMatrix());
    }

    RoxLocationProxy& getCameraProxy() { return activeCamera; }

    RoxLocation& getCamera()
    {
        if (!activeCamera.isValid())
            return RoxMemory::invalidObject<RoxLocation>();

        return activeCamera.get();
    }

} // namespace RoxScene
