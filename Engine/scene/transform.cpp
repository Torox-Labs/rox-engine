//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "transform.h"
#include "render/render.h"
#include "camera.h"
#include "memory/invalid_object.h"

namespace nya_scene
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
        return nya_memory::invalid_object<transform>();
    }

    return *active_transform;
}

nya_math::vec3 transform::transform_vec(const nya_math::vec3 &vec) const
{
    nya_math::vec3 out=vec;
    out.x*=m_scale.x;
    out.y*=m_scale.y;
    out.z*=m_scale.z;

    return m_pos+m_rot.rotate(out);
}

nya_math::quat transform::transform_quat(const nya_math::quat &quat) const
{
    return m_rot*quat;
}

nya_math::vec3 transform::inverse_transform(const nya_math::vec3 &vec) const
{
    nya_math::vec3 out=m_rot.rotate_inv(vec-m_pos);
    const float eps=0.0001f;
    out.x=fabsf(m_scale.x)>eps?out.x/m_scale.x:0.0f;
    out.y=fabsf(m_scale.y)>eps?out.y/m_scale.y:0.0f;
    out.z=fabsf(m_scale.z)>eps?out.z/m_scale.z:0.0f;

    return out;
}

nya_math::quat transform::inverse_transform(const nya_math::quat &quat) const
{
    return nya_math::quat::invert(m_rot)*quat;
}

nya_math::vec3 transform::inverse_rot(const nya_math::vec3 &vec) const
{
    return m_rot.rotate_inv(vec);
}

nya_math::vec3 transform::inverse_rot_scale(const nya_math::vec3 &vec) const
{
    nya_math::vec3 out=m_rot.rotate_inv(vec);
    const float eps=0.0001f;
    out.x=fabsf(m_scale.x)>eps?out.x/m_scale.x:0.0f;
    out.y=fabsf(m_scale.y)>eps?out.y/m_scale.y:0.0f;
    out.z=fabsf(m_scale.z)>eps?out.z/m_scale.z:0.0f;

    return out;
}

nya_math::aabb transform::transform_aabb(const nya_math::aabb &box) const
{
    return nya_math::aabb(box,m_pos,m_rot,m_scale);
}

void transform::apply() const
{
    nya_math::mat4 mat=get_camera().get_view_matrix();
    mat.translate(m_pos).rotate(m_rot).scale(m_scale.x,m_scale.y,m_scale.z);

    nya_render::set_modelview_matrix(mat);
}

void transform::set_rot(nya_math::angle_deg yaw,nya_math::angle_deg pitch,nya_math::angle_deg roll)
{
    m_rot=nya_math::quat(pitch,yaw,roll);
}

}
