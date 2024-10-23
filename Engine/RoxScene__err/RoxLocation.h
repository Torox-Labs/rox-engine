// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxProxy.h"
#include "RoxMemory/RoxTagList.h"
#include "mesh.h"
#include "RoxTags.h"
#include "transform.h"
#include <vector>
#include <cstring>

#include "RoxSharedResources.h"

namespace RoxScene
{

    struct SharedLocation
    {
        struct LocationMesh
        {
            std::string name;
            RTags tg;
            RoxTransform tr; // Assuming 'transform' is renamed to 'RoxTransform' for PascalCase
        };

        std::vector<LocationMesh> meshes;

        std::vector<std::pair<std::string, RoxMath::Vector4>> materialParams;
        std::vector<std::pair<std::string, RoxMath::Vector4>> vecParams;
        std::vector<std::pair<std::string, std::string>> stringParams;

        bool release() { *this = SharedLocation(); return true; }
    };

    class RoxLocation : public RoxSceneShared<SharedLocation>
    {
    public:
        bool load(const char* name);
        void unload();

    public:
        int addMesh(const RTags& tg, const RoxTransform& tr) { return addMesh(nullptr, tg, tr); }
        int addMesh(const char* meshName, const RTags& tg, const RoxTransform& tr);
        void removeMesh(int idx) { mMeshes.remove(idx); }

        int getMeshesCount() const { return mMeshes.getCount(); }
        const RoxMesh& getMesh(int idx) const { return mMeshes.get(idx).m; }
        RoxMesh& modifyMesh(int idx, bool needApply = true);

        int getMeshesCount(const char* tag) const { return mMeshes.getCount(tag); }
        const RoxMesh& getMesh(const char* tag, int idx) const { return mMeshes.get(tag, idx).m; }
        RoxMesh& modifyMesh(const char* tag, int idx, bool needApply = true);

        void setMeshVisible(int idx, bool visible) { mMeshes.get(idx).visible = visible; }
        bool isMeshVisible(int idx) const { return mMeshes.get(idx).visible; }

    public:
        int getMaterialParamsCount() const { return static_cast<int>(mMaterialParams.size()); }
        const char* getMaterialParamName(int idx) const;
        const RoxMaterial::ParamProxy& getMaterialParam(int idx) const;
        void setMaterialParam(int idx, const RoxMaterial::Param& p);

    public:
        void update(int dt);
        void draw(const char* pass = RoxMaterial::defaultPass, const RTags& t = RTags()) const;

    public:
        RoxLocation() : mNeedApply(false) {}
        explicit RoxLocation(const char* name) { load(name); }

    public:
        static bool loadText(SharedLocation& res, ResourceData& data, const char* name);

    private:
        struct LocationMeshInternal
        {
            RoxMesh m;
            bool visible;
            bool needApply;

            LocationMeshInternal() : visible(false), needApply(false) {}
        };

        RoxMemory::RoxTagList<LocationMeshInternal> mMeshes;
        mutable std::vector<bool> mDrawCache;
        std::vector<std::pair<std::string, RoxMaterial::ParamProxy>> mMaterialParams;
        bool mNeedApply;
    };

    using RoxLocationProxy = RoxProxy<RoxLocation>;

    // External function declarations
    void setCamera(const RoxLocationProxy& cam);
    RoxLocation& getCamera();
    RoxLocationProxy& getCameraProxy();

} // namespace RoxScene
