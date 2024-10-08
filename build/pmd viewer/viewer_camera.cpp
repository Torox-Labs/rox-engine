//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "viewer_camera.h"
#include "scene/camera.h"

void viewer_camera::add_rot(float dx,float dy)
{
    m_rot_x-=dx;
    m_rot_y-=dy;
    m_rot_x.normalize();
    m_rot_y.normalize();
    update();
}

void viewer_camera::add_pos(float dx,float dy,float dz)
{
    m_pos.x-=dx;
    m_pos.y-=dy;
    m_pos.z-=dz*(m_pos.z<50?1.0:m_pos.z/50.0f);
    if(m_pos.z<2.0f)
        m_pos.z=2.0f;

    if(m_pos.z>10000.0f)
        m_pos.z=10000.0f;

    update();
}

void viewer_camera::set_aspect(float aspect)
{
    nya_scene::get_camera().set_proj(55.0,aspect,1.0,15000.0);
    update();
}

void viewer_camera::update()
{
    nya_scene::get_camera().set_rot(m_rot_x,m_rot_y,0.0);
    nya_math::vec3 pos=nya_scene::get_camera().get_rot().rotate(m_pos);
    nya_scene::get_camera().set_pos(pos.x,pos.y+10.0f,pos.z);
}

