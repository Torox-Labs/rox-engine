//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxMath/RoxMatrix.h"
#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"
#include "RoxMath/RoxConstants.h"
#include "RoxMath/RoxFrustum.h"

namespace RoxScene
{

class transform
{
public:
    static void set(const transform &tr);
    static const transform &get();

public:
    void set_pos(float x,float y,float z) { m_pos.x=x; m_pos.y=y; m_pos.z=z; }
    void set_pos(const RoxMath::Vector3 &p) { m_pos=p; }
    void set_rot(const RoxMath::Quaternion &q) { m_rot=q; }
    void set_rot(RoxMath::AngleDeg yaw,RoxMath::AngleDeg pitch,RoxMath::AngleDeg roll);
    void set_scale(float sx,float sy,float sz) { m_scale.x=sx; m_scale.y=sy; m_scale.z=sz; }

public:
    const RoxMath::Vector3 &get_pos() const { return m_pos; }
    const RoxMath::Quaternion &get_rot() const { return m_rot; }
    const RoxMath::Vector3 &get_scale() const { return m_scale; }

public:
    RoxMath::Vector3 inverse_transform(const RoxMath::Vector3 &vec) const;
    RoxMath::Quaternion inverse_transform(const RoxMath::Quaternion &Quaternion) const;
    RoxMath::Vector3 inverse_rot(const RoxMath::Vector3 &vec) const;
    RoxMath::Vector3 inverse_rot_scale(const RoxMath::Vector3 &vec) const;
    RoxMath::Vector3 transform_vec(const RoxMath::Vector3 &vec) const;
    RoxMath::Quaternion transform_quat(const RoxMath::Quaternion &Quaternion) const;
    RoxMath::Aabb transform_aabb(const RoxMath::Aabb &box) const;

public:
    transform():m_scale(RoxMath::Vector3(1.0f,1.0f,1.0f)){}

private:
    void apply() const;

private:
    RoxMath::Vector3 m_pos;
    RoxMath::Quaternion m_rot;
    RoxMath::Vector3 m_scale;
};

}
