//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/vector.h"

class viewer_camera
{
public:
    void add_rot(float dx,float dy);
    void add_pos(float dx,float dy,float dz);

    void set_aspect(float aspect);

private:
    void update();

public:
    viewer_camera(): m_rot_x(0.0f),m_rot_y(0.0f),m_pos(0.0f,0.0f,22.0f) {}

private:
    nya_math::angle_deg m_rot_x, m_rot_y;
    nya_math::vec3 m_pos;
};
