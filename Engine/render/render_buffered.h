//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "render_api.h"
#include <queue>

namespace nya_render
{

class render_buffered: public render_api_interface
{
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

    int create_index_buffer(const void *data,vbo::index_size type,uint count,vbo::usage_hint usage) override;
    void update_index_buffer(int idx,const void *data) override;
    bool get_index_data(int idx,void *data) override;
    void remove_index_buffer(int idx) override;

public:
    int create_texture(const void *data,uint width,uint height,texture::color_format &format,int mip_count) override;
    int create_cubemap(const void *data[6],uint width,texture::color_format &format,int mip_count) override;
    void update_texture(int idx,const void *data,uint x,uint y,uint width,uint height,int mip) override;
    void set_texture_wrap(int idx,texture::wrap s,texture::wrap t) override;
    void set_texture_filter(int idx,texture::filter min,texture::filter mag,texture::filter mip,uint aniso) override;
    bool get_texture_data(int texture,uint x,uint y,uint w,uint h,void *data) override;

    void remove_texture(int texture) override;
    uint get_max_texture_dimention() override { return m_max_texture_dimention; }
    bool is_texture_format_supported(texture::color_format format) override;

public:
    virtual int create_target(uint width,uint height,uint samples,const int *attachment_textures,
                              const int *attachment_sides,uint attachment_count,int depth_texture) override;
    void resolve_target(int idx) override;
    void remove_target(int idx) override;
    uint get_max_target_attachments() override { return m_max_target_attachments; }
    uint get_max_target_msaa() override { return m_max_target_msaa; }

public:
    void set_camera(const nya_math::mat4 &modelview,const nya_math::mat4 &projection) override;
    void clear(const viewport_state &s,bool color,bool depth,bool stencil) override;
    void draw(const state &s) override;

    void invalidate_cached_state() override;
    void apply_state(const state &s) override;

public:
    size_t get_buffer_size() const { return m_current.buffer.size() * sizeof(int); }

    void commit();  //curr -> pending
    void push();    //pending -> processing
    void execute(); //run processing

public:
    render_buffered(render_api_interface &backend): m_backend(backend)
    {
        m_max_texture_dimention=m_backend.get_max_texture_dimention();
        m_max_target_attachments=m_backend.get_max_target_attachments();
        m_max_target_msaa=m_backend.get_max_target_msaa();
        for(int i=0;i<int(sizeof(m_tex_formats)/sizeof(m_tex_formats[0]));++i)
            m_tex_formats[i]=m_backend.is_texture_format_supported(texture::color_format(i));
    }

private:
    int new_idx();
    void remap_idx(int &idx) const;
    void remap_state(state &s) const;

private:
    render_api_interface &m_backend;

    uint m_max_texture_dimention,m_max_target_attachments,m_max_target_msaa;
    bool m_tex_formats[64];
    std::map<int,std::vector<shader::uniform> > m_uniform_info;
    std::map<int,uint> m_buf_sizes;

    enum command_type
    {
        cmd_clear=0x637200,
        cmd_camera,
        cmd_apply,
        cmd_draw,
        cmd_uniform,
        cmd_resolve,

        cmd_shdr_create,
        cmd_shdr_remove,
        cmd_ubuf_create,
        cmd_ubuf_remove,

        cmd_vbuf_create,
        cmd_vbuf_layout,
        cmd_vbuf_update,
        cmd_vbuf_remove,

        cmd_ibuf_create,
        cmd_ibuf_update,
        cmd_ibuf_remove,

        cmd_tex_create,
        cmd_tex_cube,
        cmd_tex_update,
        cmd_tex_wrap,
        cmd_tex_filter,
        cmd_tex_remove,

        cmd_target_create,
        cmd_target_remove,

        cmd_invalidate
    };

    struct command_buffer
    {
        void write(int command) { buffer.push_back(command); }

        template<typename t> void write(command_type command,const t &data)
        {
            const size_t offset=buffer.size();
            buffer.resize(offset+(sizeof(t)+3)/sizeof(int)+1);
            buffer[offset]=command;
            memcpy(buffer.data()+(offset+1),&data,sizeof(data));
        }

        template<typename t> void write(command_type command,const t &data,int count,const float *buf)
        {
            const size_t offset=buffer.size();
            buffer.resize(offset+(sizeof(t)+3)/sizeof(int)+1+count);
            buffer[offset]=command;
            memcpy(buffer.data()+(offset+1),&data,sizeof(data));
            memcpy(buffer.data()+(buffer.size()-count),buf,count*sizeof(float));
        }

        void write(int byte_count,const void *buf)
        {
            const size_t offset=buffer.size();
            buffer.resize(offset+(byte_count+3)/sizeof(int));
            memcpy(buffer.data()+offset,buf,byte_count);
        }

        command_type get_cmd() { return command_type(buffer[(++read_offset)-1]); }

        template<typename t> t &get_cmd_data()
        {
            const size_t size=(sizeof(t)+3)/sizeof(int);
            return *((t*)&buffer[(read_offset+=size) - size]);
        }

        float *get_fbuf(int count) { return (float *)&buffer[(read_offset+=count)-count]; }
        void *get_cbuf(int size) { const size_t s=(size+3)/sizeof(int); return &buffer[(read_offset+=s)-s]; }

        std::vector<int> buffer;
        size_t read_offset;

        std::vector<int> remap;
        std::vector<int> remap_free;
        bool update_remap;

        command_buffer():read_offset(0),update_remap(false){}
    };

    command_buffer m_current;
    command_buffer m_pending;
    command_buffer m_processing;

    struct uniform_data { int buf_idx,idx;uint count; };
    struct clear_data { viewport_state vp; bool color,depth,stencil,reserved; };
    struct camera_data { nya_math::mat4 mv,p; };
    struct shader_create_data { int idx,vs_size,ps_size; };
    struct ubuf_create_data { int idx,shader_idx; };
    struct vbuf_create_data { int idx;uint stride,count;vbo::usage_hint usage; };
    struct vbuf_layout { int idx; vbo::layout layout; };
    struct buf_update { int idx; uint size; };
    struct ibuf_create_data { int idx;vbo::index_size type; uint count; vbo::usage_hint usage; };
    struct tex_create_data { int idx; uint width,height,size; texture::color_format format; int mip_count; };
    struct tex_update { int idx; uint x,y,width,height,size; int mip; };
    struct tex_wrap { int idx; texture::wrap s,t; };
    struct tex_filter { int idx; texture::filter minification,magnification,mipmap; uint aniso; };
    struct target_create { int idx; uint w,h,s,count; int at[16]; int as[16]; int d; };
};

}
