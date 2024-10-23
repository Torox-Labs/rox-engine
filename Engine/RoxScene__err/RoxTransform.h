//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxMath/RoxMatrix.h"
#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"
#include "RoxMath/RoxConstants.h"
#include "RoxMath/RoxFrustum.h"

namespace RoxScene
{

    class RoxTransform
    {
    public:
        static void set(const RoxTransform& tr);
        static const RoxTransform& get();

    public:
        void setPos(float x, float y, float z) { m_pos.x = x; m_pos.y = y; m_pos.z = z; }
        void setPos(const RoxMath::Vector3& p) { m_pos = p; }
        void setRot(const RoxMath::Quaternion& q) { m_rot = q; }
        void setRotation(RoxMath::AngleDeg yaw, RoxMath::AngleDeg pitch, RoxMath::AngleDeg roll);
        void setScale(float sx, float sy, float sz) { m_scale.x = sx; m_scale.y = sy; m_scale.z = sz; }

    public:
        const RoxMath::Vector3& getPos() const { return m_pos; }
        const RoxMath::Quaternion& getRotation() const { return m_rot; }
        const RoxMath::Vector3& getScale() const { return m_scale; }

    public:
        RoxMath::Vector3 inverseTransform(const RoxMath::Vector3& vec) const;
        RoxMath::Quaternion inverseTransform(const RoxMath::Quaternion& Quaternion) const;
        RoxMath::Vector3 inverseRotation(const RoxMath::Vector3& vec) const;
        RoxMath::Vector3 inverseRotScale(const RoxMath::Vector3& vec) const;
        RoxMath::Vector3 transformVector(const RoxMath::Vector3& vec) const;
        RoxMath::Quaternion transformQuaternion(const RoxMath::Quaternion& Quaternion) const;
        RoxMath::Aabb transformAabb(const RoxMath::Aabb& box) const;

    public:
        RoxTransform() :m_scale(RoxMath::Vector3(1.0f, 1.0f, 1.0f)) {}

    private:
        void apply() const;

    private:
        RoxMath::Vector3 m_pos;
        RoxMath::Quaternion m_rot;
        RoxMath::Vector3 m_scale;
    };

}
