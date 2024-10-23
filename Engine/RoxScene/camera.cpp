//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "camera.h"
#include "RoxRender/RoxRender.h"
#include "RoxMath/RoxQuaternion.h"
#include "RoxMath/RoxConstants.h"
#include "RoxRender/RoxTransform.h"
#include "RoxMemory/RoxInvalidObject.h"
#include <cmath>

namespace RoxScene
{

namespace { camera_proxy active_camera=camera_proxy(camera()); }

void camera::set_proj(RoxMath::AngleDeg fov,float aspect,float near,float far)
{
    m_proj.identity();
    m_proj.perspective(fov,aspect,near,far);
    if(this==active_camera.operator->())
        RoxRender::setProjectionMatrix(m_proj);

    m_recalc_frustum=true;
}

void camera::set_proj(float left,float right,float bottom,float top,float near,float far)
{
    m_proj.identity();
    m_proj.ortho(left,right,bottom,top,near,far);
    if(this==active_camera.operator->())
        RoxRender::setProjectionMatrix(m_proj);

    m_recalc_frustum=true;
}

void camera::set_proj(const RoxMath::Matrix4 &mat)
{
    m_proj=mat;
    if(this==active_camera.operator->())
        RoxRender::setProjectionMatrix(m_proj);

    m_recalc_frustum=true;
}

void camera::set_pos(const RoxMath::Vector3 &pos)
{
    m_pos=pos;
    m_recalc_view=true;
    m_recalc_frustum=true;
}

void camera::set_rot(const RoxMath::Quaternion &rot)
{
    m_rot=rot;
    m_recalc_view=true;
    m_recalc_frustum=true;
}

void camera::set_rot(RoxMath::AngleDeg yaw,RoxMath::AngleDeg pitch,RoxMath::AngleDeg roll)
{
    set_rot(RoxMath::Quaternion(pitch,yaw,roll));
}

void camera::set_rot(const RoxMath::Vector3 &dir)
{
    const float eps=1.0e-6f;
    const RoxMath::Vector3 v=RoxMath::Vector3::normalize(dir);
    const float xz_sqdist=v.x*v.x+v.z*v.z;

    const float new_yaw=(xz_sqdist>eps*eps)? (std::atan2(v.x,v.z)+RoxMath::Constants::pi) : m_rot.getEuler().y;
    const float new_pitch=(fabsf(v.y)>eps)? (std::atan2(v.y,sqrtf(xz_sqdist))) : 0.0f;
    set_rot(RoxMath::Quaternion(new_pitch,new_yaw,0));
}

const RoxMath::Matrix4 &camera::get_view_matrix() const
{
    if(m_recalc_view)
    {
        m_recalc_view=false;
        m_view=RoxMath::Matrix4(m_rot);
        m_view.transpose();
        m_view.translate(-m_pos);
    }

    return m_view;
}

const RoxMath::RoxFrustum &camera::get_frustum() const
{
    if(m_recalc_frustum)
    {
        m_recalc_frustum=false;

        if(RoxRender::RoxTransform::get().hasOrientationMatrix())
            m_frustum=RoxMath::RoxFrustum(get_view_matrix()*get_proj_matrix()*RoxRender::RoxTransform::get().getOrientationMatrix());
        else
            m_frustum=RoxMath::RoxFrustum(get_view_matrix()*get_proj_matrix());
    }

    return m_frustum;
}

const RoxMath::Vector3 camera::get_dir() const
{
    return m_rot.rotate(RoxMath::Vector3::forward());
}

void set_camera(const camera_proxy &cam)
{
    active_camera=cam;
    if(cam.is_valid())
        RoxRender::setProjectionMatrix(cam->get_proj_matrix());
}

camera_proxy &get_camera_proxy() { return active_camera; }

camera &get_camera()
{
    if(!active_camera.is_valid())
        return RoxMemory::invalidObject<camera>();

    return active_camera.get();
}

}
