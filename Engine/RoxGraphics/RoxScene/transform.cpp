//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "transform.h"
#include "RoxRender/RoxRender.h"
#include "camera.h"
#include "RoxMemory/RoxInvalidObject.h"

namespace RoxScene
{

namespace { const transform *active_transform=0; }

void transform::set(const transform &tr)
{
    active_transform=&tr;
    tr.apply();
}

const transform &transform::get()
{
    if(!active_transform)
    {
        return RoxMemory::invalidObject<transform>();
    }

    return *active_transform;
}

RoxMath::Vector3 transform::transform_vec(const RoxMath::Vector3 &vec) const
{
    RoxMath::Vector3 out=vec;
    out.x*=m_scale.x;
    out.y*=m_scale.y;
    out.z*=m_scale.z;

    return m_pos+m_rot.rotate(out);
}

RoxMath::Quaternion transform::transform_quat(const RoxMath::Quaternion &quat) const
{
    return m_rot*quat;
}

RoxMath::Vector3 transform::inverse_transform(const RoxMath::Vector3 &vec) const
{
    RoxMath::Vector3 out=m_rot.rotateInv(vec-m_pos);
    const float eps=0.0001f;
    out.x=fabsf(m_scale.x)>eps?out.x/m_scale.x:0.0f;
    out.y=fabsf(m_scale.y)>eps?out.y/m_scale.y:0.0f;
    out.z=fabsf(m_scale.z)>eps?out.z/m_scale.z:0.0f;

    return out;
}

RoxMath::Quaternion transform::inverse_transform(const RoxMath::Quaternion &quat) const
{
    return RoxMath::Quaternion::invert(m_rot)*quat;
}

RoxMath::Vector3 transform::inverse_rot(const RoxMath::Vector3 &vec) const
{
    return m_rot.rotateInv(vec);
}

RoxMath::Vector3 transform::inverse_rot_scale(const RoxMath::Vector3 &vec) const
{
    RoxMath::Vector3 out=m_rot.rotateInv(vec);
    const float eps=0.0001f;
    out.x=fabsf(m_scale.x)>eps?out.x/m_scale.x:0.0f;
    out.y=fabsf(m_scale.y)>eps?out.y/m_scale.y:0.0f;
    out.z=fabsf(m_scale.z)>eps?out.z/m_scale.z:0.0f;

    return out;
}

RoxMath::Aabb transform::transform_aabb(const RoxMath::Aabb &box) const
{
    return RoxMath::Aabb(box,m_pos,m_rot,m_scale);
}

void transform::apply() const
{
    RoxMath::Matrix4 mat=get_camera().get_view_matrix();
    mat.translate(m_pos).rotate(m_rot).scale(m_scale.x,m_scale.y,m_scale.z);

    RoxRender::setModelViewMatrix(mat);
}

void transform::set_rot(RoxMath::AngleDeg yaw,RoxMath::AngleDeg pitch,RoxMath::AngleDeg roll)
{
    m_rot=RoxMath::Quaternion(pitch,yaw,roll);
}

}
