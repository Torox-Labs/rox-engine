//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "texture.h"
#include "bitmap.h"
#include "render_opengl.h"
#include "memory/tmp_buffer.h"

namespace nya_render
{

namespace
{
    texture::wrap default_wrap_s=texture::wrap_repeat;
    texture::wrap default_wrap_t=texture::wrap_repeat;
    texture::filter default_min_filter=texture::filter_linear;
    texture::filter default_mag_filter=texture::filter_linear;
    texture::filter default_mip_filter=texture::filter_linear;
    unsigned int default_aniso=0;
}

bool texture::build_texture(const void *data_a[6],bool is_cubemap,unsigned int width,unsigned int height,
                            color_format format,int mip_count)
{
    if(width==0 || height==0)
    {
        log()<<"Unable to build texture: invalid width or height\n";
        release();
        return false;
    }

    if(width>get_max_dimension() || height>get_max_dimension())
    {
        log()<<"Unable to build texture: width or height is too high, maximum is "<<get_max_dimension()<<"\n";
        release();
        return false;
    }

    if(is_cubemap && width!=height)
    {
        release();
        return false;
    }

    const void *data=data_a?data_a[0]:0;

    if(format>=dxt1)
    {
        if(!data || mip_count==0)
        {
            log()<<"Unable to build texture: compressed format with invalid data\n";
            release();
            return false;
        }
    }

    if(format>=color_r32f && mip_count<0)
        mip_count=1;

    const bool is_pvrtc=format==pvr_rgb2b || format==pvr_rgba2b || format==pvr_rgb4b || format==pvr_rgba4b;
    const bool pot=((width&(width-1))==0 && (height&(height-1))==0);

    if(is_pvrtc && !(width==height && pot))
    {
        log()<<"Unable to build texture: pvr compression supports only square pot textures\n";
        release();
        return false;
    }

    if(!is_cubemap && !m_is_cubemap && m_width==width && m_height==height && m_format==format && data)
    {
        get_api_interface().update_texture(m_tex,data,0,0,width,height,-1);
        return true;
    }
    else
        release();

    if(!data)
        mip_count=0;
    else if(!pot)
        mip_count=1;

    nya_memory::tmp_buffer_ref tmp_buf;

    if(!get_api_interface().is_texture_format_supported(format))
    {
        if(format==depth24)
        {
            if(data)
                return false; //ToDo

            format=depth32;
        }
        else if(format==color_bgra || format==color_rgb)
        {
            if(!get_api_interface().is_texture_format_supported(color_rgba))
                return false;

            if(data)
            {
                size_t size=0;
                for(int i=0,w=width,h=height;i<(mip_count>=0?mip_count:1);w>1?w=w/2:w=1,h>1?h/=2:h=1,++i)
                    size+=width*height*4;

                tmp_buf.allocate(is_cubemap?size*6:size);
                for(int i=0;i<(is_cubemap?6:1);++i)
                {
                    const unsigned char *from=(unsigned char *)(is_cubemap?data_a[i]:data);
                    unsigned char *to=(unsigned char *)tmp_buf.get_data(i*size);
                    (is_cubemap?data_a[i]:data)=to;
                    if(format==color_rgb)
                    {
                        for(size_t j=0;j<size;j+=4,from+=3,to+=4)
                        {
                            memcpy(to,from,3);
                            to[3]=255;
                        }
                    }
                    else if(format==color_bgra)
                    {
                        for(size_t j=0;j<size;j+=4)
                        {
                            to[j]=from[j+2];
                            to[j+1]=from[j+1];
                            to[j+2]=from[j];
                            to[j+3]=from[j+3];
                        }
                    }
                }
            }
            format=color_rgba;
        }
        else
        {
            log()<<"Unable to build texture: unsupported format\n";
            return false;
        }
    }

    if(is_cubemap)
        m_tex=get_api_interface().create_cubemap(data_a,width,format,mip_count);
    else
        m_tex=get_api_interface().create_texture(data,width,height,format,mip_count);

    tmp_buf.free();

    if(m_tex<0)
        return false;

    m_width=width,m_height=height;
    m_format=format;
    m_is_cubemap=is_cubemap;
    if(!m_filter_set)
    {
        m_filter_min=default_min_filter;
        m_filter_mag=default_mag_filter;
        m_filter_mip=default_mip_filter;
    }
    if(!m_aniso_set)
        m_aniso=default_aniso;
    get_api_interface().set_texture_filter(m_tex,m_filter_min,m_filter_mag,m_filter_mip,m_aniso);

    if(!is_cubemap)
    {
        const bool force_clamp=!pot && !is_platform_restrictions_ignored();
        get_api_interface().set_texture_wrap(m_tex,force_clamp?wrap_clamp:default_wrap_s,force_clamp?wrap_clamp:default_wrap_t);
    }
    return true;
}

bool texture::build_texture(const void *data,unsigned int width,unsigned int height,color_format format,int mip_count)
{
    const void *data_a[6]={data};
    return build_texture(data_a,false,width,height,format,mip_count);
}

bool texture::build_cubemap(const void *data[6],unsigned int width,unsigned int height,color_format format,int mip_count)
{
    return build_texture(data,true,width,height,format,mip_count);
}

bool texture::update_region(const void *data,unsigned int x,unsigned int y,unsigned int width,unsigned int height,int mip)
{
    if(m_tex<0 && m_is_cubemap)
        return false;

    unsigned int mip_width=mip<0?m_width:m_width>>mip, mip_height=mip<0?m_height:m_height>>mip;
    if(!mip_width) mip_width=1;
    if(!mip_height) mip_height=1;

    if(x+width>mip_width || y+height>mip_height)
        return false;

    if(m_format>=depth16)
        return false;

    get_api_interface().update_texture(m_tex,data,x,y,width,height,-1);
    return true;
}

bool texture::copy_region(const texture &src,uint src_x,uint src_y,uint width,uint height,uint dst_x,uint dst_y)
{
    if(src.m_tex<0 || m_tex<0)
        return false;
/*
    const texture_obj &f=texture_obj::get(src.m_tex);
    texture_obj &t=texture_obj::get(m_tex);

    if(f.is_cubemap || t.is_cubemap)
        return false;

    if(src_x+width>f.width || src_y+height>f.height)
        return false;

    if(dst_x+width>t.width || dst_y+height>t.height)
        return false;

    if(t.format>=dxt1)
        return false;

    if(m_tex==src.m_tex)
        return src_x==dst_x && src_y==dst_y;
*/
    //renderapi

    return false;
}

void texture::bind(unsigned int layer) const
{
    if(layer>=render_api_interface::state::max_layers)
        return;

    get_api_state().textures[layer]=m_tex;
}

void texture::unbind(unsigned int layer)
{
    if(layer>=render_api_interface::state::max_layers)
        return;

    get_api_state().textures[layer]=-1;
}

bool texture::get_data(nya_memory::tmp_buffer_ref &data) const
{
    return get_data(data,0,0,m_width,m_height);
}

bool texture::get_data(nya_memory::tmp_buffer_ref &data,unsigned int x,unsigned int y,unsigned int w,unsigned int h) const
{
    if(m_tex<0 || !w || !h || x+w>m_width || y+h>m_height)
        return false;

    size_t size=w*h;
    switch(m_format)
    {
        case greyscale: break;
        case color_rgb: size*=3; break;

        case color_r32f: break;
        case color_rgb32f: size*=3*sizeof(float); break;
        case color_rgba32f: size*=4*sizeof(float); break;

        case depth16: break; size*=2; break;
        case depth24: break; size*=3; break;

        default: size*=4;
    }

    data.allocate(size);
    if(!get_api_interface().get_texture_data(m_tex,x,y,w,h,data.get_data()))
    {
        data.free();
        return false;
    }
    return true;
}

unsigned int texture::get_width() const { return m_width; }
unsigned int texture::get_height() const { return m_height; }
texture::color_format texture::get_color_format() const { return m_format; }
bool texture::is_cubemap() const { return m_is_cubemap; }

void texture::set_wrap(wrap s,wrap t)
{
    if(m_tex<0 || m_is_cubemap)
        return;

    const bool pot=((m_width&(m_width-1))==0 && (m_height&(m_height-1))==0);
    if(!pot)
        s=t=wrap_clamp;

    get_api_interface().set_texture_wrap(m_tex,s,t);
}

void texture::set_aniso(unsigned int level)
{
    m_aniso=level;
    m_aniso_set=true;

    if(m_tex<0)
        return;

    get_api_interface().set_texture_filter(m_tex,m_filter_min,m_filter_mag,m_filter_mip,m_aniso);
}

void texture::set_filter(filter minification,filter magnification,filter mipmap)
{
    m_filter_min=minification;
    m_filter_mag=magnification;
    m_filter_mip=mipmap;
    m_filter_set=true;

    if(m_tex<0)
        return;

    get_api_interface().set_texture_filter(m_tex,m_filter_min,m_filter_mag,m_filter_mip,m_aniso);
}

void texture::set_default_wrap(wrap s,wrap t)
{
    default_wrap_s=s;
    default_wrap_t=t;
}

void texture::set_default_filter(filter minification,filter magnification,filter mipmap)
{
    default_min_filter=minification;
    default_mag_filter=magnification;
    default_mip_filter=mipmap;
}

void texture::set_default_aniso(unsigned int level) { default_aniso=level; }

void texture::get_default_wrap(wrap &s,wrap &t)
{
    s=default_wrap_s;
    t=default_wrap_t;
}

void texture::get_default_filter(filter &minification,filter &magnification,filter &mipmap)
{
    minification=default_min_filter;
    magnification=default_mag_filter;
    mipmap=default_mip_filter;
}

unsigned int texture::get_default_aniso() { return default_aniso; }
unsigned int texture::get_max_dimension() { return get_api_interface().get_max_texture_dimention(); }
bool texture::is_dxt_supported() { return get_api_interface().is_texture_format_supported(dxt1); }

unsigned int texture::get_format_bpp(texture::color_format format)
{
    switch(format)
    {
        case texture::color_bgra: return 32;
        case texture::color_rgba: return 32;
        case texture::color_rgb: return 24;
        case texture::greyscale: return 8;
        case texture::color_rgba32f: return 32*4;
        case texture::color_rgb32f: return 32*3;
        case texture::color_r32f: return 32;
        case texture::depth16: return 16;
        case texture::depth24: return 24;
        case texture::depth32: return 32;

        case texture::dxt1: return 4;
        case texture::dxt3: return 8;
        case texture::dxt5: return 8;

        case texture::etc1: return 4;
        case texture::etc2: return 4;
        case texture::etc2_a1: return 4;
        case texture::etc2_eac: return 8;

        case texture::pvr_rgb2b: return 2;
        case texture::pvr_rgb4b: return 4;
        case texture::pvr_rgba2b: return 2;
        case texture::pvr_rgba4b: return 4;
    };

    return 0;
}

ID3D11Texture2D *texture::get_dx11_tex_id() const
{
    //renderapi

    return 0;
}

unsigned int texture::get_gl_tex_id() const
{
    if(m_tex<0)
        return 0;

    if(get_render_api()==render_api_opengl)
        return render_opengl::get().get_gl_texture_id(m_tex);
    
    //renderapi

    return 0;
}

unsigned int texture::get_used_vmem_size()
{
    //renderapi

    return 0;
}

void texture::release()
{
    if(m_tex<0)
        return;

    get_api_interface().remove_texture(m_tex);
    render_api_interface::state &s=get_api_state();
    for(int i=0;i<s.max_layers;++i)
    {
        if(s.textures[i]==m_tex)
            s.textures[i]=-1;
    }

    *this=texture();
}

}
