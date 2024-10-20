//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxRenderApi.h"
#include <queue>

namespace RoxRender
{

class RoxRenderBuffered: public RoxRenderApiInterface
{
public:
    int create_shader(const char *vertex,const char *fragment) override;
    uint get_uniforms_count(int shader) override;
    RoxShader::uniform get_uniform(int shader,int idx) override;
    void remove_shader(int shader) override;

    int create_uniform_buffer(int shader) override;
    void set_uniform(int uniform_buffer,int idx,const float *buf,uint count) override;
    void remove_uniform_buffer(int uniform_buffer) override;

public:
    int create_vertex_buffer(const void *data,uint stride,uint count,RoxVbo::usage_hint usage) override;
    void set_vertex_layout(int idx,RoxVbo::Layout layout) override;
    void update_vertex_buffer(int idx,const void *data) override;
    bool get_vertex_data(int idx,void *data) override;
    void remove_vertex_buffer(int idx) override;

    int create_index_buffer(const void *data,RoxVbo::index_size type,uint count,RoxVbo::usage_hint usage) override;
    void update_index_buffer(int idx,const void *data) override;
    bool get_index_data(int idx,void *data) override;
    void remove_index_buffer(int idx) override;

public:
    int create_texture(const void *data,uint width,uint height,RoxTexture::COLOR_FORMAT &format,int mip_count) override;
    int create_cubemap(const void *data[6],uint width,RoxTexture::COLOR_FORMAT &format,int mip_count) override;
    void update_texture(int idx,const void *data,uint x,uint y,uint width,uint height,int mip) override;
    void set_texture_wrap(int idx,RoxTexture::wrap s,RoxTexture::wrap t) override;
    void set_texture_filter(int idx,RoxTexture::filter min,RoxTexture::filter mag,RoxTexture::filter mip,uint aniso) override;
    bool get_texture_data(int RoxTexture,uint x,uint y,uint w,uint h,void *data) override;

    void remove_texture(int RoxTexture) override;
    uint get_max_texture_dimention() override { return m_max_texture_dimention; }
    bool is_texture_format_supported(RoxTexture::COLOR_FORMAT format) override;

public:
    virtual int create_target(uint width,uint height,uint samples,const int *attachment_textures,
                              const int *attachment_sides,uint attachment_count,int depth_texture) override;
    void resolve_target(int idx) override;
    void remove_target(int idx) override;
    uint get_max_target_attachments() override { return m_max_target_attachments; }
    uint get_max_target_msaa() override { return m_max_target_msaa; }

public:
    void set_camera(const RoxMath::Matrix4 &modelview,const RoxMath::Matrix4 &projection) override;
    void clear(const ViewportState &s,bool color,bool depth,bool stencil) override;
    void draw(const state &s) override;

    void invalidate_cached_state() override;
    void apply_state(const state &s) override;

public:
    size_t get_buffer_size() const { return m_current.buffer.size() * sizeof(int); }

    void commit();  //curr -> pending
    void push();    //pending -> processing
    void execute(); //run processing

public:
    RoxRenderBuffered(RoxRenderApiInterface &backend): m_backend(backend)
    {
        m_max_texture_dimention=m_backend.getMaxTextureDimention();
        m_max_target_attachments=m_backend.getMaxTargetAttachments();
        m_max_target_msaa=m_backend.getMaxTargetMsaa();
        for(int i=0;i<int(sizeof(m_tex_formats)/sizeof(m_tex_formats[0]));++i)
            m_tex_formats[i]=m_backend.isTextureFormatSupported(RoxTexture::COLOR_FORMAT(i));
    }

private:
    int new_idx();
    void remap_idx(int &idx) const;
    void remap_state(state &s) const;

private:
    RoxRenderApiInterface &m_backend;

    uint m_max_texture_dimention,m_max_target_attachments,m_max_target_msaa;
    bool m_tex_formats[64];
    std::map<int,std::vector<RoxShader::uniform> > m_uniform_info;
    std::map<int,uint> m_buf_sizes;

	enum COMMAND_TYPE
	{
		CMD_CLEAR = 0x637200,
		CMD_CAMERA,
		CMD_APPLY,
		CMD_DRAW,
		CMD_UNIFORM,
		CMD_RESOLVE,

		CMD_SHDR_CREATE,
		CMD_SHDR_REMOVE,
		CMD_UBUF_CREATE,
		CMD_UBUF_REMOVE,

		CMD_VBUF_CREATE,
		CMD_VBUF_LAYOUT,
		CMD_VBUF_UPDATE,
		CMD_VBUF_REMOVE,

		CMD_IBUF_CREATE,
		CMD_IBUF_UPDATE,
		CMD_IBUF_REMOVE,

		CMD_TEX_CREATE,
		CMD_TEX_CUBE,
		CMD_TEX_UPDATE,
		CMD_TEX_WRAP,
		CMD_TEX_FILTER,
		CMD_TEX_REMOVE,

		CMD_TARGET_CREATE,
		CMD_TARGET_REMOVE,

		CMD_INVALIDATE
	};

    struct CommandBuffer
    {
        void write(int command) { buffer.push_back(command); }

        template<typename t> void write(COMMAND_TYPE command,const t &data)
        {
            const size_t offset=buffer.size();
            buffer.resize(offset+(sizeof(t)+3)/sizeof(int)+1);
            buffer[offset]=command;
            memcpy(buffer.data()+(offset+1),&data,sizeof(data));
        }

        template<typename t> void write(COMMAND_TYPE command,const t &data,int count,const float *buf)
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

        COMMAND_TYPE get_cmd() { return COMMAND_TYPE(buffer[(++read_offset)-1]); }

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

        CommandBuffer():read_offset(0),update_remap(false){}
    };

    CommandBuffer m_current;
    CommandBuffer m_pending;
    CommandBuffer m_processing;

    struct uniform_data { int buf_idx,idx;uint count; };
    struct clear_data { ViewportState vp; bool color,depth,stencil,reserved; };
    struct camera_data { RoxMath::Matrix4 mv,p; };
    struct shader_create_data { int idx,vs_size,ps_size; };
    struct ubuf_create_data { int idx,shader_idx; };
    struct vbuf_create_data { int idx;uint stride,count;RoxVbo::UsageHint usage; };
    struct vbuf_layout { int idx; RoxVbo::Layout layout; };
    struct buf_update { int idx; uint size; };
    struct ibuf_create_data { int idx;RoxVbo::IndexSize type; uint count; RoxVbo::UsageHint usage; };
    struct tex_create_data { int idx; uint width,height,size; RoxTexture::COLOR_FORMAT format; int mip_count; };
    struct tex_update { int idx; uint x,y,width,height,size; int mip; };
    struct tex_wrap { int idx; RoxTexture::wrap s,t; };
    struct tex_filter { int idx; RoxTexture::filter minification,magnification,mipmap; uint aniso; };
    struct target_create { int idx; uint w,h,s,count; int at[16]; int as[16]; int d; };
};

}
