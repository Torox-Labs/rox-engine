// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Update the render api intefrace to check Metal 1th.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include "RoxRenderApi.h"

#if __APPLE__ && __OBJC__
  @protocol MTLDevice,CAMetalDrawable,MTLTexture;
#endif

namespace RoxRender
{

class RoxRenderMetal: public RoxRenderApiInterface
{
public:
    bool isAvailable() const override;

#ifdef __APPLE__
public:
    int create_shader(const char *vertex,const char *fragment) override;
    uint get_uniforms_count(int shader) override;
    shader::uniform get_uniform(int shader,int idx) override;
    int create_uniform_buffer(int shader) override;
    void set_uniform(int uniform_buffer,int idx,const float *buf,uint count) override;
    void remove_uniform_buffer(int uniform_buffer) override;
    void remove_shader(int shader) override;

public:
    int create_vertex_buffer(const void *data,uint stride,uint count,vbo::usage_hint usage) override;
    void update_vertex_buffer(int idx,const void *data) override;
    void set_vertex_layout(int idx,vbo::layout layout) override;
    void remove_vertex_buffer(int idx) override;
    int create_index_buffer(const void *data,vbo::index_size size,uint indices_count,vbo::usage_hint usage) override;
    void remove_index_buffer(int idx) override;

public:
    int create_texture(const void *data,uint width,uint height,texture::color_format &format,int mip_count) override;
    int create_cubemap(const void *data[6],uint width,texture::color_format &format,int mip_count) override;
    void update_texture(int idx,const void *data,uint x,uint y,uint width,uint height,int mip) override;
    bool get_texture_data(int texture,uint x,uint y,uint w,uint h,void *data) override;
    void remove_texture(int texture) override;
    uint get_max_texture_dimention() override;
    bool is_texture_format_supported(texture::color_format format) override;

public:
    void set_camera(const nya_math::mat4 &modelview,const nya_math::mat4 &projection) override;
    void clear(const viewport_state &s,bool color,bool depth,bool stencil) override;
    void draw(const state &s) override;

    void invalidate_cached_state() override;
    void apply_state(const state &s) override;
#endif

#if __APPLE__ && __OBJC__
    static void set_device(id<MTLDevice> device);
    static void start_frame(id<CAMetalDrawable> drawable,id<MTLTexture> depth);
    static void end_frame();
#endif

public:
    static RoxRenderMetal &get();

    //ToDo: set device

private:
    RoxRenderMetal() {}
};

}
