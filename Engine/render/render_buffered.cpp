//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "render_buffered.h"

namespace nya_render
{

bool render_buffered::get_vertex_data(int idx,void *data)
{
    log()<<"get_vertex_data is not supported in render_buffered\n";
    return false;
}

bool render_buffered::get_index_data(int idx,void *data)
{
    log()<<"get_index_data is not supported in render_buffered\n";
    return false;
}

bool render_buffered::get_texture_data(int texture,uint x,uint y,uint w,uint h,void *data)
{
    log()<<"get_texture_data is not supported in render_buffered\n";
    return false;
}

//----------------------------------------------------------------

void render_buffered::set_uniform(int uniform_buffer,int idx,const float *buf,uint count)
{
    uniform_data d;
    d.buf_idx=uniform_buffer,d.idx=idx,d.count=count;
    m_current.write(cmd_uniform,d,count,buf);
}

void render_buffered::set_camera(const nya_math::mat4 &modelview,const nya_math::mat4 &projection)
{
    camera_data d;
    d.mv=modelview;
    d.p=projection;
    m_current.write(cmd_camera,d);
}

void render_buffered::clear(const viewport_state &s,bool color,bool depth,bool stencil)
{
    clear_data d;
    d.vp=s,d.color=color,d.depth=depth,d.stencil=stencil;
    m_current.write(cmd_clear,d);
}

void render_buffered::draw(const state &s) { m_current.write(cmd_draw,s); }
void render_buffered::apply_state(const state &s) { m_current.write(cmd_apply,s); }
void render_buffered::resolve_target(int idx) { m_current.write(cmd_resolve,idx); }

int render_buffered::create_shader(const char *vertex,const char *fragment)
{
    //ToDo
    std::vector<shader::uniform> uniforms;
    for(int i=0;i<2;++i)
    {
        shader_code_parser p(!i?vertex:fragment);
        for(int i=0;i<p.get_uniforms_count();++i)
        {
            const shader_code_parser::variable &v=p.get_uniform(i);
            if(v.type==shader_code_parser::type_mat4 &&
               (v.name=="_nya_ModelViewMatrix" || v.name=="_nya_ProjectionMatrix" || v.name=="_nya_ModelViewProjectionMatrix"))
                continue;

            bool found=false;
            for(int j=0;j<(int)uniforms.size();++j)
            {
                if(uniforms[j].name==v.name)
                {
                    found=true;
                    break;
                }
            }
            if(found)
                continue;

            shader::uniform u;
            u.name=v.name;
            u.type=(shader::uniform_type)v.type;
            u.array_size=v.array_size;
            uniforms.push_back(u);
        }
    }

    shader_create_data d;
    d.idx=new_idx();
    d.vs_size=(int)strlen(vertex)+1;
    d.ps_size=(int)strlen(fragment)+1;

    m_uniform_info[d.idx]=uniforms;

    m_current.write(cmd_shdr_create,d);
    m_current.write(d.vs_size,vertex);
    m_current.write(d.ps_size,fragment);
    return d.idx;
}

render_buffered::uint render_buffered::get_uniforms_count(int shader) { return (int)m_uniform_info[shader].size(); }
shader::uniform render_buffered::get_uniform(int shader,int idx) { return m_uniform_info[shader][idx]; }
void render_buffered::remove_shader(int shader) { m_current.write(cmd_shdr_remove,shader); }

int render_buffered::create_uniform_buffer(int shader)
{
    ubuf_create_data d;
    d.idx=new_idx();
    d.shader_idx=shader;
    m_current.write(cmd_ubuf_create,d);
    return d.idx;
}

void render_buffered::remove_uniform_buffer(int uniform_buffer) { m_current.write(cmd_ubuf_remove,uniform_buffer); }

int render_buffered::create_vertex_buffer(const void *data,uint stride,uint count,vbo::usage_hint usage)
{
    vbuf_create_data d;
    d.idx=new_idx();
    d.stride=stride;
    d.count=count;
    d.usage=usage;
    m_current.write(cmd_vbuf_create,d);
    m_current.write(stride*count,data);
    m_buf_sizes[d.idx]=stride*count;
    return d.idx;
}

void render_buffered::set_vertex_layout(int idx,vbo::layout layout)
{
    vbuf_layout d;
    d.idx=idx;
    d.layout=layout;
    m_current.write(cmd_vbuf_layout,d);
}

void render_buffered::update_vertex_buffer(int idx,const void *data)
{
    buf_update d;
    d.idx=idx;
    d.size=m_buf_sizes[idx];
    m_current.write(cmd_vbuf_update,d);
    m_current.write(d.size,data);
}

void render_buffered::remove_vertex_buffer(int idx) { m_current.write(cmd_vbuf_remove,idx); }

int render_buffered::create_index_buffer(const void *data,vbo::index_size type,uint count,vbo::usage_hint usage)
{
    ibuf_create_data d;
    d.idx=new_idx();
    d.type=type;
    d.count=count;
    m_current.write(cmd_ibuf_create,d);
    m_current.write(type*count,data);
    m_buf_sizes[d.idx]=type*count;
    return d.idx;
}

void render_buffered::update_index_buffer(int idx,const void *data)
{
    buf_update d;
    d.idx=idx;
    d.size=m_buf_sizes[idx];
    m_current.write(cmd_ibuf_update,d);
    m_current.write(d.size,data);
}

void render_buffered::remove_index_buffer(int idx) { m_current.write(cmd_ibuf_remove,idx); }

const int texture_size(unsigned int width,unsigned int height,texture::color_format &format,int mip_count)
{
    if(!mip_count)
        return 0;

    if(mip_count<0)
        mip_count=1;

    const bool is_pvrtc=format==texture::pvr_rgb2b || format==texture::pvr_rgba2b || format==texture::pvr_rgb4b || format==texture::pvr_rgba4b;
    const unsigned int bpp=texture::get_format_bpp(format);
    unsigned int size=0;

    for(unsigned int i=0,w=width,h=height;i<(unsigned int)(mip_count>0?mip_count:1);++i,w=w>1?w/2:1,h=h>1?h/2:1)
    {
        if(format<texture::dxt1)
            size+=w*h*bpp/8;
        else
        {
            if(is_pvrtc)
            {
                if(format==texture::pvr_rgb2b || format==texture::pvr_rgba2b)
                    size+=((w>16?w:16)*(h>8?h:8)*2 + 7)/8;
                else
                    size+=((w>8?w:8)*(h>8?h:8)*4 + 7)/8;
            }
            else
                size+=(w>4?w:4)/4 * (h>4?h:4)/4 * bpp*2;
        }
    }

    return size;
}

int render_buffered::create_texture(const void *data,uint width,uint height,texture::color_format &format,int mip_count)
{
    //ToDo: backend may modify format

    tex_create_data d;
    d.idx=new_idx();
    d.width=width;
    d.height=height;
    d.size=texture_size(width,height,format,mip_count);
    d.format=format;
    d.mip_count=mip_count;

    m_current.write(cmd_tex_create,d);
    if(d.size>0)
        m_current.write(d.size,data);

    m_buf_sizes[d.idx]=texture::get_format_bpp(format);;
    return d.idx;
}

int render_buffered::create_cubemap(const void *data[6],uint width,texture::color_format &format,int mip_count)
{
    //ToDo: backend may modify format

    tex_create_data d;
    d.idx=new_idx();
    d.width=width;
    d.height=width;
    d.size=texture_size(width,width,format,mip_count);
    d.format=format;
    d.mip_count=mip_count;

    m_current.write(cmd_tex_cube,d);
    if(d.size>0)
    {
        for(int i=0;i<6;++i)
            m_current.write(d.size,(char*)data[i]);
    }

    m_buf_sizes[d.idx]=texture::get_format_bpp(format);;
    return d.idx;
}

void render_buffered::update_texture(int idx,const void *data,uint x,uint y,uint width,uint height,int mip)
{
    tex_update d;
    d.idx=idx;
    d.x=x,d.y=y,d.width=width,d.height=height;
    const uint bpp=m_buf_sizes[idx];
    for(uint i=0,w=width,h=height;i<uint(mip>0?mip:1);++i,w=w>1?w/2:1,h=h>1?h/2:1)
        d.size=w*h*bpp/8;
    d.mip=mip;

    m_current.write(cmd_tex_update,d);
    m_current.write(d.size,data);
}

void render_buffered::set_texture_wrap(int idx,texture::wrap s,texture::wrap t)
{
    tex_wrap d;
    d.idx=idx;
    d.s=s,d.t=t;
    m_current.write(cmd_tex_wrap,d);
}

void render_buffered::set_texture_filter(int idx,texture::filter minification,texture::filter magnification,texture::filter mipmap,uint aniso)
{
    tex_filter d;
    d.idx=idx;
    d.minification=minification;
    d.magnification=magnification;
    d.mipmap=mipmap;
    d.aniso=aniso;
    m_current.write(cmd_tex_filter,d);
}

void render_buffered::remove_texture(int texture) { m_current.write(cmd_tex_remove,texture); }
bool render_buffered::is_texture_format_supported(texture::color_format format) { return m_tex_formats[format]; }

int render_buffered::create_target(uint width,uint height,uint samples,const int *attachment_textures,
                                   const int *attachment_sides,uint attachment_count,int depth_texture)
{
    target_create d;
    d.idx=new_idx();
    d.w=width;
    d.h=height;
    d.s=samples;
    d.count=attachment_count;
    for(uint i=0;i<attachment_count;++i)
    {
        d.at[i]=attachment_textures[i];
        d.as[i]=attachment_sides[i];
    }
    d.d=depth_texture;
    m_current.write(cmd_target_create,d);
    return d.idx;
}

void render_buffered::remove_target(int idx) { m_current.write(cmd_target_remove,idx); }
void render_buffered::invalidate_cached_state() { m_current.write(cmd_invalidate); }

//----------------------------------------------------------------

static const int invalid_idx= -1;

//----------------------------------------------------------------

int render_buffered::new_idx()
{
    if(!m_current.remap_free.empty())
    {
        const int idx=m_current.remap_free.back();
        m_current.remap_free.pop_back();
        return idx;
    }

    const int idx=(int)m_current.remap.size();
    m_current.remap.push_back(invalid_idx);
    m_current.update_remap=true;
    return idx;
}

void render_buffered::commit()
{
    m_current.buffer.swap(m_pending.buffer);

    if(m_pending.update_remap)
    {
        if(!m_pending.remap_free.empty())
        {
            m_current.remap_free.insert(m_current.remap_free.end(),m_pending.remap_free.begin(),m_pending.remap_free.end());
            m_pending.remap_free.clear();
        }

        memcpy(&m_current.remap[0],&m_pending.remap[0],m_pending.remap.size()*sizeof(m_pending.remap[0]));
        m_pending.update_remap=false;
    }

    if(m_current.update_remap)
    {
        m_pending.remap=m_current.remap;
        m_current.update_remap=false;
        m_pending.update_remap=true;
    }
}

void render_buffered::push()
{
    m_pending.buffer.swap(m_processing.buffer);

    bool pending_changed=false;

    if(m_processing.update_remap)
    {
        memcpy(&m_pending.remap[0],&m_processing.remap[0],m_processing.remap.size()*sizeof(m_processing.remap[0]));
        m_processing.update_remap=false;
        pending_changed=true;

        if(!m_processing.remap_free.empty())
        {
            m_pending.remap_free.swap(m_processing.remap_free);
            m_processing.remap_free.clear();
        }
    }

    if(m_pending.update_remap)
        m_processing.remap=m_pending.remap;

    m_pending.update_remap=pending_changed;
}

void render_buffered::remap_idx(int &idx) const { if(idx>=0) { idx=m_processing.remap[idx]; } }

void render_buffered::remap_state(state &s) const
{
    remap_idx(s.target);
    remap_idx(s.vertex_buffer);
    remap_idx(s.index_buffer);
    remap_idx(s.shader);
    remap_idx(s.uniform_buffer);
    for(int i=0;i<s.max_layers;++i)
        remap_idx(s.textures[i]);
}

//----------------------------------------------------------------

void render_buffered::execute()
{
    m_processing.read_offset = 0;
    while(m_processing.read_offset<m_processing.buffer.size())
    {
        command_type cmd=m_processing.get_cmd();
        switch (cmd)
        {
            case cmd_uniform:
            {
                uniform_data d=m_processing.get_cmd_data<uniform_data>();
                d.buf_idx=m_processing.remap[d.buf_idx];
                if(d.buf_idx<0)
                    m_processing.get_fbuf(d.count);
                else
                    m_backend.set_uniform(d.buf_idx,d.idx,m_processing.get_fbuf(d.count),d.count);
                break;
            }

            case cmd_draw:
            {
                state &s=m_processing.get_cmd_data<state>();
                remap_state(s);
                m_backend.draw(s);
                break;
            }

            case cmd_camera:
            {
                camera_data &d=m_processing.get_cmd_data<camera_data>();
                m_backend.set_camera(d.mv,d.p);
                break;
            }

            case cmd_clear:
            {
                clear_data &d=m_processing.get_cmd_data<clear_data>();
                remap_idx(d.vp.target);
                m_backend.clear(d.vp,d.color,d.depth,d.stencil);
                break;
            }

            case cmd_resolve:
            {
                int idx=m_processing.get_cmd_data<int>();
                remap_idx(idx);
                m_backend.resolve_target(idx);
                break;
            }

            case cmd_apply:
            {
                state &s=m_processing.get_cmd_data<state>();
                remap_state(s);
                m_backend.apply_state(s);
                break;
            }

            case cmd_shdr_create:
            {
                const shader_create_data d=m_processing.get_cmd_data<shader_create_data>();
                const char *vs=(char *)m_processing.get_cbuf(d.vs_size);
                const char *ps=(char *)m_processing.get_cbuf(d.ps_size);
                m_processing.remap[d.idx]=m_backend.create_shader(vs,ps);
                m_processing.update_remap=true;
                break;
            }

            case cmd_shdr_remove:
            {
                const int idx=m_processing.get_cmd_data<int>();
                const int ridx=m_processing.remap[idx];
                if(ridx<0)
                    break;

                m_backend.remove_shader(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap=true;
                break;
            }

            case cmd_ubuf_create:
            {
                ubuf_create_data &d=m_processing.get_cmd_data<ubuf_create_data>();
                remap_idx(d.shader_idx);
                m_processing.remap[d.idx]=m_backend.create_uniform_buffer(d.shader_idx);
                m_processing.update_remap=true;
                break;
            }

            case cmd_ubuf_remove:
            {
                const int idx=m_processing.get_cmd_data<int>();
                const int ridx=m_processing.remap[idx];
                if(ridx<0)
                    break;

                m_backend.remove_uniform_buffer(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap=true;
                break;
            }

            case cmd_vbuf_create:
            {
                const vbuf_create_data d=m_processing.get_cmd_data<vbuf_create_data>();
                const void *data=m_processing.get_cbuf(d.count*d.stride);
                m_processing.remap[d.idx]=m_backend.create_vertex_buffer(data,d.stride,d.count,d.usage);
                m_processing.update_remap=true;
                break;
            }

            case cmd_vbuf_layout:
            {
                vbuf_layout &d=m_processing.get_cmd_data<vbuf_layout>();
                remap_idx(d.idx);
                if(d.idx>=0)
                    m_backend.set_vertex_layout(d.idx,d.layout);
                break;
            }

            case cmd_vbuf_remove:
            {
                const int idx=m_processing.get_cmd_data<int>();
                const int ridx=m_processing.remap[idx];
                if(ridx<0)
                    break;

                m_backend.remove_vertex_buffer(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap=true;
                break;
            }

            case cmd_ibuf_create:
            {
                const ibuf_create_data d=m_processing.get_cmd_data<ibuf_create_data>();
                const void *data=m_processing.get_cbuf(d.count*d.type);
                m_processing.remap[d.idx]=m_backend.create_index_buffer(data,d.type,d.count,d.usage);
                m_processing.update_remap=true;
                break;
            }

            case cmd_ibuf_remove:
            {
                const int idx=m_processing.get_cmd_data<int>();
                const int ridx=m_processing.remap[idx];
                if(ridx<0)
                    break;

                m_backend.remove_index_buffer(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap=true;
                break;
            }

            case cmd_tex_create:
            {
                tex_create_data d=m_processing.get_cmd_data<tex_create_data>();
                const void *data=d.size>0?m_processing.get_cbuf(d.size):0;
                m_processing.remap[d.idx]=m_backend.create_texture(data,d.width,d.height,d.format,d.mip_count);
                m_processing.update_remap=true;
                break;
            }

            case cmd_tex_cube:
            {
                tex_create_data d=m_processing.get_cmd_data<tex_create_data>();
                const void *data[6]={0};
                if(d.size>0)
                {
                    for(int i=0;i<6;++i)
                        data[i]=m_processing.get_cbuf(d.size);
                }
                m_processing.remap[d.idx]=m_backend.create_cubemap(data,d.width,d.format,d.mip_count);
                m_processing.update_remap=true;
                break;
            }

            case cmd_tex_update:
            {
                tex_update d=m_processing.get_cmd_data<tex_update>();
                remap_idx(d.idx);
                const void *buf=m_processing.get_cbuf(d.size);
                if(d.idx>=0)
                    m_backend.update_texture(d.idx,buf,d.x,d.y,d.width,d.height,d.mip);
                break;
            }

            case cmd_tex_wrap:
            {
                tex_wrap &d=m_processing.get_cmd_data<tex_wrap>();
                remap_idx(d.idx);
                if(d.idx>=0)
                    m_backend.set_texture_wrap(d.idx,d.s,d.t);
                break;
            }

            case cmd_tex_filter:
            {
                tex_filter &d=m_processing.get_cmd_data<tex_filter>();
                remap_idx(d.idx);
                if(d.idx>=0)
                    m_backend.set_texture_filter(d.idx,d.minification,d.magnification,d.mipmap,d.aniso);
                break;
            }

            case cmd_tex_remove:
            {
                const int idx=m_processing.get_cmd_data<int>();
                const int ridx=m_processing.remap[idx];
                if(ridx<0)
                    break;

                m_backend.remove_texture(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap=true;
                break;
            }

            case cmd_target_create:
            {
                target_create &d=m_processing.get_cmd_data<target_create>();
                for(uint i=0;i<d.count;++i)
                    remap_idx(d.at[i]);
                remap_idx(d.d);
                m_processing.remap[d.idx]=m_backend.create_target(d.w,d.h,d.s,d.at,d.as,d.count,d.d);
                m_processing.update_remap=true;
                break;
            }

            case cmd_target_remove:
            {
                const int idx=m_processing.get_cmd_data<int>();
                const int ridx=m_processing.remap[idx];
                if(ridx<0)
                    break;

                m_backend.remove_target(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap=true;
                break;
            }

            default:
                log()<<"unsupported render command: "<<cmd<<"\n"; m_processing.buffer.clear(); return;
        }
    }

    m_processing.buffer.clear();
}

}
