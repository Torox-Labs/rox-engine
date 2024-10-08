//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "scene/material.h"
#include "math/quaternion.h"

namespace vr
{
    class IVRSystem;
    struct HmdMatrix34_t;
}

namespace nya_system
{

enum vr_eye
{
    vr_eye_left,
    vr_eye_right
};

enum vr_colorspace
{
    vr_colorspace_auto,
    vr_colorspace_gamma,
    vr_colorspace_linear,
};

bool disable_vsync();

bool submit_texture(vr_eye eye,const nya_render::texture &tex,vr_colorspace=vr_colorspace_auto);
bool submit_texture(vr_eye eye,const nya_scene::texture &tex,vr_colorspace=vr_colorspace_auto);
bool submit_texture(vr_eye eye,const nya_scene::texture_proxy &tex,vr_colorspace=vr_colorspace_auto);

nya_math::mat4 get_proj_matrix(vr::IVRSystem *hmd,vr_eye eye, float znear, float zfar);

nya_math::vec3 get_pos(const vr::HmdMatrix34_t &mat);
nya_math::quat get_rot(const vr::HmdMatrix34_t &mat);

}
