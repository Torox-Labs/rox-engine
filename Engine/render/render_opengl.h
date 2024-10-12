//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "render_api.h"

namespace nya_render
{

class render_opengl: public render_api_interface
{
public:
    bool is_available() const override;

public:
    void invalidate_cached_state() override;
    void apply_state(const state &s) override;

public:
    int create_shader(const char *vertex,const char *fragment) override;
    uint get_uniforms_count(int shader) override;
    shader::uniform get_uniform(int shader,int idx) override;
    void remove_shader(int shader) override;

    int create_uniform_buffer(int shader) override;
    void set_uniform(int uniform_buffer,int idx,const float *buf,uint count) override;
    void remove_uniform_buffer(int uniform_buffer) override;

public:
    int create_vertex_buffer(const void *data,uint stride,uint count,vbo::usage_hint usage) override;
    void set_vertex_layout(int idx,vbo::layout layout) override;
    void update_vertex_buffer(int idx,const void *data) override;
    bool get_vertex_data(int idx,void *data) override;
    void remove_vertex_buffer(int idx) override;

    int create_index_buffer(const void *data,vbo::index_size size,uint indices_count,vbo::usage_hint usage) override;
    void update_index_buffer(int idx,const void *data) override;
    bool get_index_data(int idx,void *data) override;
    void remove_index_buffer(int idx) override;

public:
    int create_texture(const void *data,uint width,uint height,texture::color_format &format,int mip_count) override;
    int create_cubemap(const void *data[6],uint width,texture::color_format &format,int mip_count) override;
    void update_texture(int idx,const void *data,uint x,uint y,uint width,uint height,int mip) override;
    void set_texture_wrap(int idx,texture::wrap s,texture::wrap t) override;
    void set_texture_filter(int idx,texture::filter minification,texture::filter magnification,texture::filter mipmap,uint aniso) override;
    bool get_texture_data(int texture,uint x,uint y,uint w,uint h,void *data) override;
    void remove_texture(int texture) override;
    uint get_max_texture_dimention() override;
    bool is_texture_format_supported(texture::color_format format) override;

public:
    int create_target(uint width,uint height,uint samples,const int *attachment_textures,
                      const int *attachment_sides,uint attachment_count,int depth_texture) override;
    void resolve_target(int idx) override;
    void remove_target(int idx) override;
    uint get_max_target_attachments() override;
    uint get_max_target_msaa() override;

public:
    void set_camera(const nya_math::mat4 &modelview,const nya_math::mat4 &projection) override;
    void clear(const viewport_state &s,bool color,bool depth,bool stencil) override;
    void draw(const state &s) override;
    void transform_feedback(const tf_state &s) override;
    bool is_transform_feedback_supported() override;

public:
    static void log_errors(const char *place=0);
    static void enable_debug(bool synchronous);

    static bool has_extension(const char *name);
    static void *get_extension(const char *name);

    uint get_gl_texture_id(int idx);

    //for external textures
    static void gl_bind_texture(uint gl_type,uint gl_idx,uint layer=0);
    static void gl_bind_texture2d(uint gl_idx,uint layer=0);

public:
    static render_opengl &get();

private:
    render_opengl() {}
};

}
