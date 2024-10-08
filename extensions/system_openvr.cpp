//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//requires: openvr https://github.com/ValveSoftware/openvr

#include "system_openvr.h"
#include "memory/invalid_object.h"
#include "render/render_opengl.h"
#include <openvr.h>

#ifdef _WIN32
    #include "windows.h"
#endif

namespace nya_system
{

bool disable_vsync()
{
#ifdef _WIN32
    typedef BOOL (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int interval);
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if(!wglSwapIntervalEXT)
        return false;

    wglSwapIntervalEXT(0);
    return true;
#else
    return false;
#endif
}

inline vr::EVREye to_eye(vr_eye eye) { return eye==vr_eye_left?vr::EVREye::Eye_Left:vr::EVREye::Eye_Right; }

bool submit_texture(vr_eye eye,const nya_render::texture &tex,vr_colorspace colorspace)
{
    if(!vr::VRCompositor())
        return false;

    vr::EColorSpace vrcolorspace=vr::ColorSpace_Auto;
    if(colorspace==vr_colorspace_gamma)
        vrcolorspace=vr::ColorSpace_Gamma;
    else if(colorspace==vr_colorspace_linear)
        vrcolorspace=vr::ColorSpace_Linear;

    if(nya_render::get_render_api()==nya_render::render_api_directx11)
    {
        vr::Texture_t vr_tex={tex.get_dx11_tex_id(),vr::TextureType_DirectX,vrcolorspace};
        return vr::VRCompositor()->Submit(to_eye(eye),&vr_tex)==vr::VRCompositorError_None;
    }

    if(nya_render::get_render_api()==nya_render::render_api_opengl)
    {
        vr::Texture_t vr_tex={(void*)(ptrdiff_t)tex.get_gl_tex_id(),vr::TextureType_OpenGL,vrcolorspace};
        vr::EVRCompositorError result=vr::VRCompositor()->Submit(to_eye(eye),&vr_tex);
        nya_render::render_opengl::gl_bind_texture2d(0); //resets cache
        return result==vr::VRCompositorError_None;
    }

    return false;
}

bool submit_texture(vr_eye eye,const nya_scene::texture &tex,vr_colorspace colorspace)
{
    if(!tex.internal().get_shared_data().is_valid())
        return false;

    return submit_texture(eye,tex.internal().get_shared_data()->tex,colorspace);
}

bool submit_texture(vr_eye eye,const nya_scene::texture_proxy &tex,vr_colorspace colorspace)
{
    if(!tex.is_valid())
        return false;

    return submit_texture(eye,tex.get(),colorspace);
}

nya_math::mat4 get_proj_matrix(vr::IVRSystem *hmd,vr_eye eye,float znear,float zfar)
{
    nya_math::mat4 mat;
    if(!hmd)
        return mat;

    vr::HmdMatrix44_t vr_mat = hmd->GetProjectionMatrix(to_eye(eye), znear, zfar);
    memcpy(&mat, &vr_mat, sizeof(vr_mat));
    mat.transpose();
    return mat;
}

nya_math::vec3 get_pos(const vr::HmdMatrix34_t &mat)
{
    //return nya_math::mat4(mat.m).transpose().get_pos();

    return nya_math::vec3(mat.m[0][3],mat.m[1][3],mat.m[2][3]);
}

nya_math::quat get_rot(const vr::HmdMatrix34_t &mat)
{
    return nya_math::mat4(mat.m).get_rot();
}

}
