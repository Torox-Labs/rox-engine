//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "camera.h"
#include "render/render.h"
#include "math/quaternion.h"
#include "math/constants.h"
#include "render/transform.h"
#include "memory/invalid_object.h"
#include <cmath>

namespace nya_scene
{

namespace { camera_proxy active_camera=camera_proxy(camera()); }

void camera::set_proj(nya_math::angle_deg fov,float aspect,float near,float far)
{
    m_proj.identity();
    m_proj.perspective(fov,aspect,near,far);
    if(this==active_camera.operator->())
        nya_render::set_projection_matrix(m_proj);

    m_recalc_frustum=true;
}

void camera::set_proj(float left,float right,float bottom,float top,float near,float far)
{
    m_proj.identity();
    m_proj.ortho(left,right,bottom,top,near,far);
    if(this==active_camera.operator->())
        nya_render::set_projection_matrix(m_proj);

    m_recalc_frustum=true;
}

void camera::set_proj(const nya_math::mat4 &mat)
{
    m_proj=mat;
    if(this==active_camera.operator->())
        nya_render::set_projection_matrix(m_proj);

    m_recalc_frustum=true;
}

void camera::set_pos(const nya_math::vec3 &pos)
{
    m_pos=pos;
    m_recalc_view=true;
    m_recalc_frustum=true;
}

void camera::set_rot(const nya_math::quat &rot)
{
    m_rot=rot;
    m_recalc_view=true;
    m_recalc_frustum=true;
}

void camera::set_rot(nya_math::angle_deg yaw,nya_math::angle_deg pitch,nya_math::angle_deg roll)
{
    set_rot(nya_math::quat(pitch,yaw,roll));
}

void camera::set_rot(const nya_math::vec3 &dir)
{
    const float eps=1.0e-6f;
    const nya_math::vec3 v=nya_math::vec3::normalize(dir);
    const float xz_sqdist=v.x*v.x+v.z*v.z;

    const float new_yaw=(xz_sqdist>eps*eps)? (std::atan2(v.x,v.z)+nya_math::constants::pi) : m_rot.get_euler().y;
    const float new_pitch=(fabsf(v.y)>eps)? (std::atan2(v.y,sqrtf(xz_sqdist))) : 0.0f;
    set_rot(nya_math::quat(new_pitch,new_yaw,0));
}

const nya_math::mat4 &camera::get_view_matrix() const
{
    if(m_recalc_view)
    {
        m_recalc_view=false;
        m_view=nya_math::mat4(m_rot);
        m_view.transpose();
        m_view.translate(-m_pos);
    }

    return m_view;
}

const nya_math::frustum &camera::get_frustum() const
{
    if(m_recalc_frustum)
    {
        m_recalc_frustum=false;

        if(nya_render::transform::get().has_orientation_matrix())
            m_frustum=nya_math::frustum(get_view_matrix()*get_proj_matrix()*nya_render::transform::get().get_orientation_matrix());
        else
            m_frustum=nya_math::frustum(get_view_matrix()*get_proj_matrix());
    }

    return m_frustum;
}

const nya_math::vec3 camera::get_dir() const
{
    return m_rot.rotate(nya_math::vec3::forward());
}

void set_camera(const camera_proxy &cam)
{
    active_camera=cam;
    if(cam.is_valid())
        nya_render::set_projection_matrix(cam->get_proj_matrix());
}

camera_proxy &get_camera_proxy() { return active_camera; }

camera &get_camera()
{
    if(!active_camera.is_valid())
        return nya_memory::invalid_object<camera>();

    return active_camera.get();
}

}
