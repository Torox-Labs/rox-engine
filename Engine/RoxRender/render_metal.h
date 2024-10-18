//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "render_api.h"

#if __APPLE__ && __OBJC__
  @protocol MTLDevice,CAMetalDrawable,MTLTexture;
#endif

namespace nya_render
{

class render_metal: public render_api_interface
{
public:
    bool is_available() const override;

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
    static render_metal &get();

    //ToDo: set device

private:
    render_metal() {}
};

}
