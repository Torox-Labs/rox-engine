//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxTransform.h"
#include "RoxRender/RoxRender.h"
#include "RoxCamera.h"
#include "RoxMemory/RoxInvalidObject.h"

namespace RoxScene
{

    namespace { const RoxTransform* active_transform = 0; }

    void RoxTransform::set(const RoxTransform& tr)
    {
        active_transform = &tr;
        tr.apply();
    }

    const RoxTransform& RoxTransform::get()
    {
        if (!active_transform)
        {
            return RoxMemory::invalidObject<RoxTransform>();
        }

        return *active_transform;
    }

    RoxMath::Vector3 RoxTransform::transformVector(const RoxMath::Vector3& vec) const
    {
        RoxMath::Vector3 out = vec;
        out.x *= m_scale.x;
        out.y *= m_scale.y;
        out.z *= m_scale.z;

        return m_pos + m_rot.rotate(out);
    }

    RoxMath::Quaternion RoxTransform::transformQuaternion(const RoxMath::Quaternion& Quaternion) const
    {
        return m_rot * Quaternion;
    }

    RoxMath::Vector3 RoxTransform::inverseTransform(const RoxMath::Vector3& vec) const
    {
        RoxMath::Vector3 out = m_rot.rotateInv(vec - m_pos);
        const float eps = 0.0001f;
        out.x = fabsf(m_scale.x) > eps ? out.x / m_scale.x : 0.0f;
        out.y = fabsf(m_scale.y) > eps ? out.y / m_scale.y : 0.0f;
        out.z = fabsf(m_scale.z) > eps ? out.z / m_scale.z : 0.0f;

        return out;
    }

    RoxMath::Quaternion RoxTransform::inverseTransform(const RoxMath::Quaternion& Quaternion) const
    {
        return RoxMath::Quaternion::invert(m_rot) * Quaternion;
    }

    RoxMath::Vector3 RoxTransform::inverseRotation(const RoxMath::Vector3& vec) const
    {
        return m_rot.rotateInv(vec);
    }

    RoxMath::Vector3 RoxTransform::inverseRotScale(const RoxMath::Vector3& vec) const
    {
        RoxMath::Vector3 out = m_rot.rotateInv(vec);
        const float eps = 0.0001f;
        out.x = fabsf(m_scale.x) > eps ? out.x / m_scale.x : 0.0f;
        out.y = fabsf(m_scale.y) > eps ? out.y / m_scale.y : 0.0f;
        out.z = fabsf(m_scale.z) > eps ? out.z / m_scale.z : 0.0f;

        return out;
    }

    RoxMath::Aabb RoxTransform::transformAabb(const RoxMath::Aabb& box) const
    {
        return RoxMath::Aabb(box, m_pos, m_rot, m_scale);
    }

    void RoxTransform::apply() const
    {
        RoxMath::Matrix4 mat = RoxScene::getCamera().getViewMatrix();
        mat.translate(m_pos).rotate(m_rot).scale(m_scale.x, m_scale.y, m_scale.z);

        RoxRender::setModelviewMatrix(mat);
    }

    void RoxTransform::setRotation(RoxMath::AngleDeg yaw, RoxMath::AngleDeg pitch, RoxMath::AngleDeg roll)
    {
        m_rot = RoxMath::Quaternion(pitch, yaw, roll);
    }

}
