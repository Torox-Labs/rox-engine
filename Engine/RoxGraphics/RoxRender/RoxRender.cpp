// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Dropping Metal graphic API support, and by this we are dropping Mac support for now.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxLogger/RoxLogger.h"
#include "RoxRenderOpengl.h"

#include "RoxTexture.h"
#include "RoxTransform.h"
#include "RoxMath/RoxVector.h"

#include <map>

namespace RoxRender
{

namespace
{
    RoxLogger::RoxLoggerBase *render_log=0;
    IRoxRenderApi::State current_state;

    IRoxRenderApi *availableRenderInterface()
    {

#if __APPLE__
        //TODO: Add Metal Support
#endif

#if WIN32
		//TODO: Add DirectX Support
#endif

        if (RoxRenderOpengl::get().isAvailable())
            return &RoxRenderOpengl::get();

        return 0;
    }

    IRoxRenderApi *render_interface = availableRenderInterface();
}

void set_log(RoxLogger::RoxLoggerBase *l) { render_log=l; }
RoxLogger::RoxLoggerBase &log()
{
    if(!render_log)
        return RoxLogger::log();

    return *render_log;
}

void setViewport(const Rectangle &r) { current_state.viewport=r; }
void setViewport(int x,int y,int width,int height)
{
    current_state.viewport.x=x;
    current_state.viewport.y=y;
    current_state.viewport.width=width;
    current_state.viewport.height=height;
}
const Rectangle &getViewport() { return current_state.viewport; }

void setProjectionMatrix(const RoxMath::Matrix4 &mat)
{
    RoxTransform::get().setProjectionMatrix(mat);
    getApiInterface().setCamera(getModelViewMatrix(),getProjectionMatrix());
}

void setModelViewMatrix(const RoxMath::Matrix4 &mat)
{
    RoxTransform::get().setModelviewMatrix(mat);
    getApiInterface().setCamera(getModelViewMatrix(),getProjectionMatrix());
}

void setOrientationMatrix(const RoxMath::Matrix4 &mat)
{
    RoxTransform::get().setOrientationMatrix(mat);
    getApiInterface().setCamera(getModelViewMatrix(),getProjectionMatrix());
}

const RoxMath::Matrix4 &getProjectionMatrix() { return RoxTransform::get().getProjectionMatrix(); }
const RoxMath::Matrix4 &getModelViewMatrix() { return RoxTransform::get().getModelviewMatrix(); }
const RoxMath::Matrix4 &getOrientationMatrix() { return RoxTransform::get().getOrientationMatrix(); }

RoxMath::Vector4 getClearColor() { return RoxMath::Vector4(current_state.clear_color); }
void setClearColor(const RoxMath::Vector4 &c) { setClearColor(c.x, c.y, c.z, c.w); }
void setClearColor(float r,float g,float b,float a)
{
    current_state.clear_color[0]=r;
    current_state.clear_color[1]=g;
    current_state.clear_color[2]=b;
    current_state.clear_color[3]=a;
}

float getClearDepth() { return current_state.clear_depth; }
void setClearDepth(float value) { current_state.clear_depth=value; }
void clear(bool color,bool depth,bool stencil) { render_interface->clear(current_state,color,depth,stencil); }

void Blend::enable(Blend::MODE src,Blend::MODE dst)
{
    current_state.blend=true;
    current_state.blend_src=src;
    current_state.blend_dst=dst;
}
void Blend::disable() { current_state.blend=false; }

void CullFace::enable(CullFace::ORDER o)
{
    current_state.cull_face=true;
    current_state.cull_order=o;
}
void CullFace::disable() { current_state.cull_face=false; }

void DepthTest::enable(COMPARISON MODE)
{
    current_state.depth_test=true;
    current_state.depth_comparison=MODE;
}
void DepthTest::disable() { current_state.depth_test =false; }

void Zwrite::enable() { current_state.zwrite=true; }
void Zwrite::disable() { current_state.zwrite=false; }

void ColorWrite::enable() { current_state.color_write=true; }
void ColorWrite::disable() { current_state.color_write=false; }

void Scissor::enable(const Rectangle &r) { current_state.scissor_enabled=true; current_state.scissor=r; }
void Scissor::enable(int x,int y,int w,int h)
{
    current_state.scissor_enabled=true;
    current_state.scissor.x=x;
    current_state.scissor.y=y;
    current_state.scissor.width=w;
    current_state.scissor.height=h;
}
void Scissor::disable() { current_state.scissor_enabled=false; }
bool Scissor::isEnabled() { return current_state.scissor_enabled; }
const Rectangle &Scissor::get() { return current_state.scissor; }

void setState(const State &s) { *((State *)&current_state)=s; }
const State &getState() { return current_state; }

void applyState(bool ignore_cache)
{
    if (ignore_cache)
        render_interface->invalidateCachedState();

    render_interface->applyState(current_state);
}

    // TODO: Add support to Metal,DirectX, and Vulkan
RrenderApi getRenderApi()
{
    if (render_interface == &RoxRenderOpengl::get())
        return RENDER_API_OPENGL;

    return RENDER_API_CUSTOM;
}

// TODO: Add support to Metal,DirectX, and Vulkan
bool setRenderApi(RrenderApi api)
{
    switch(api)
    {
        case RENDER_API_OPENGL: return setRenderApi(&RoxRenderOpengl::get());
        case RENDER_API_CUSTOM: return false;
    }
    return false;
}

bool setRenderApi(IRoxRenderApi *api)
{
    if (!api || !api->isAvailable())
        return false;
    
    render_interface = api;
    return true;
}

IRoxRenderApi::State &getApiState() { return current_state; }
IRoxRenderApi &getApiInterface() { return *render_interface; }

namespace { bool ignorePlatformRestrictions=true; }
void setIgnorePlatformRestrictions(bool ignore) { ignorePlatformRestrictions =ignore; }
bool isPlatformRestrictionsIgnored() { return ignorePlatformRestrictions; }
}
