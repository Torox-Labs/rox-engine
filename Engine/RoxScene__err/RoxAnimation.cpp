// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxAnimation.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxFormats/RoxAnim.h"

namespace RoxScene
{

    bool RoxAnimation::load(const char* name)
    {
        defaultLoadFunction(loadNan);

        if (!RoxSceneShared<SharedAnimation>::load(name))
            return false;

        if (!mShared.isValid())
            return false;

        mRangeFrom = 0;
        mRangeTo = mShared->anim.getDuration();
        mSpeed = mWeight = 1.0f;
        updateVersion();
        mMask.free();

        return true;
    }

    void RoxAnimation::unload()
    {
        RoxSceneShared<SharedAnimation>::unload();

        mRangeFrom = mRangeTo = 0;
        mSpeed = mWeight = 1.0f;

        mMask.free();
    }

    void RoxAnimation::create(const SharedAnimation& res)
    {
        RoxSceneShared::create(res);

        mRangeFrom = 0;
        mRangeTo = mShared->anim.getDuration();
        mSpeed = mWeight = 1.0f;
        updateVersion();
        mMask.free();
    }

    bool RoxAnimation::loadNan(SharedAnimation& res, ResourceData& data, const char* name)
    {
        RoxFormats::RoxAnim roxAnim;
        if (!roxAnim.read(data.getData(), data.getSize()))
            return false;

        res.anim.release();
        for (size_t i = 0; i < roxAnim.posVec3LinearCurves.size(); ++i)
        {
            const int boneIdx = res.anim.addBone(roxAnim.posVec3LinearCurves[i].boneName.c_str());
            for (size_t j = 0; j < roxAnim.posVec3LinearCurves[i].frames.size(); ++j)
            {
                const RoxFormats::RoxAnim::PosVec3LinearFrame& f = roxAnim.posVec3LinearCurves[i].frames[j];
                res.anim.addBonePosFrame(boneIdx, f.time, f.pos);
            }
        }

        for (size_t i = 0; i < roxAnim.rotQuatLinearCurves.size(); ++i)
        {
            const int boneIdx = res.anim.addBone(roxAnim.rotQuatLinearCurves[i].boneName.c_str());
            for (size_t j = 0; j < roxAnim.rotQuatLinearCurves[i].frames.size(); ++j)
            {
                const RoxFormats::RoxAnim::RotQuatLinearFrame& f = roxAnim.rotQuatLinearCurves[i].frames[j];
                res.anim.addBoneRotFrame(boneIdx, f.time, f.rot);
            }
        }

        for (size_t i = 0; i < roxAnim.floatLinearCurves.size(); ++i)
        {
            const int curveIdx = res.anim.addCurve(roxAnim.floatLinearCurves[i].boneName.c_str());
            for (size_t j = 0; j < roxAnim.floatLinearCurves[i].frames.size(); ++j)
            {
                const RoxFormats::RoxAnim::FloatLinearFrame& f = roxAnim.floatLinearCurves[i].frames[j];
                res.anim.addCurveFrame(curveIdx, f.time, f.value);
            }
        }

        return true;
    }

    unsigned int RoxAnimation::getDuration() const
    {
        if (!mShared.isValid())
            return 0;

        return mShared->anim.getDuration();
    }

    void RoxAnimation::setRange(unsigned int from, unsigned int to)
    {
        if (!mShared.isValid())
            return;

        mRangeFrom = from;
        mRangeTo = to;

        unsigned int duration = mShared->anim.getDuration();
        if (mRangeFrom > duration)
            mRangeFrom = duration;

        if (mRangeTo > duration)
            mRangeTo = duration;

        if (mRangeFrom > mRangeTo)
            mRangeFrom = mRangeTo;
    }

    void RoxAnimation::maskAll(bool enabled)
    {
        if (enabled)
        {
            if (mMask.isValid())
            {
                mMask.free();
                updateVersion();
            }
        }
        else
        {
            if (!mMask.isValid())
                mMask.allocate();
            else
                mMask->data.clear();

            updateVersion();
        }
    }

    void RoxAnimation::updateVersion()
    {
        static unsigned int version = 0;
        mVersion = ++version;
    }

    void RoxAnimation::addMask(const char* name, bool enabled)
    {
        if (!name)
            return;

        if (!mShared.isValid())
            return;

        if (mShared->anim.getBoneIdx(name) < 0)
            return;

        if (enabled)
        {
            if (!mMask.isValid())
                return;

            mMask->data[name] = true;
            updateVersion();
        }
        else
        {
            if (!mMask.isValid())
            {
                mMask.allocate();
                for (int i = 0; i < mShared->anim.getBonesCount(); ++i)
                    mMask->data[mShared->anim.getBoneName(i)] = true;
            }

            auto it = mMask->data.find(name);
            if (it != mMask->data.end())
                mMask->data.erase(it);

            updateVersion();
        }
    }

} // namespace RoxScene
