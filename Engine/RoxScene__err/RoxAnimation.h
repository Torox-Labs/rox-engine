// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxSharedResources.h"
#include "RoxRender/RoxAnimation.h"
#include "RoxMemory/RoxOptional.h"
#include <string>
#include <map>

namespace RoxScene { class MeshInternal; }

namespace RoxScene
{

    struct SharedAnimation
    {
        RoxRender::RoxAnimation anim;

        bool release()
        {
            anim.release();
            return true;
        }
    };

    class RoxAnimation : public RoxSceneShared<SharedAnimation>
    {
        friend class MeshInternal;

    public:
        bool load(const char* name);
        void unload();

    public:
        void create(const SharedAnimation& res);

    public:
        void setRange(unsigned int from, unsigned int to);
        void setWeight(float weight) { mWeight = weight; }
        void setSpeed(float speed) { mSpeed = speed; }
        void setLoop(bool looped) { mLooped = looped; }

        unsigned int getDuration() const;
        float getWeight() const { return mWeight; }
        float getSpeed() const { return mSpeed; }
        bool getLoop() const { return mLooped; }

    public:
        void maskAll(bool enabled);
        void addMask(const char* name, bool enabled);

    private:
        void updateVersion();

    public:
        RoxAnimation()
            : mLooped(true), mRangeFrom(0), mRangeTo(0), mSpeed(1.0f), mWeight(1.0f), mVersion(0) {}
        RoxAnimation(const char* name) { *this = RoxAnimation(); load(name); }

    public:
        static bool loadNan(SharedAnimation& res, ResourceData& data, const char* name);

    private:
        bool mLooped;
        unsigned int mRangeFrom;
        unsigned int mRangeTo;
        float mSpeed;
        float mWeight;

        unsigned int mVersion;

        struct MaskData { std::map<std::string, bool> data; };

        RoxMemory::RoxOptional<MaskData> mMask;
    };

} // namespace RoxScene
