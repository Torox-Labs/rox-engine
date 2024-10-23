//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxTexture.h"
#include "scene.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxMemory/RoxTmpBuffers.h"
#include "RoxFormats/RoxTruevisionGraphicsAdapter.h"
#include "RoxFormats/RoxDirectDrawSurface.h"
#include "RoxFormats/RoxKhronosTexture.h"
#include "RoxFormats/RoxMeta.h"
#include "RoxRender/RoxFbo.h"
#include "RoxRender/RoxShader.h"
#include "RoxRender/RoxScreenQuad.h"
#include "RoxRender/RoxBitmap.h"
#include <stdlib.h>

namespace RoxScene
{

int RoxTexture::m_load_ktx_mip_offset=0;

bool RoxTexture::load_ktx(shared_texture &res,resource_data &data,const char* name)
{
    if(!data.get_size())
        return false;

    if(data.get_size()<12)
        return false;

    if(memcmp((const char *)data.get_data()+1,"KTX ",4)!=0)
        return false;

    RoxFormats::ktx ktx;
    const size_t header_size=ktx.decode_header(data.get_data(),data.get_size());
    if(!header_size)
    {
        log()<<"unable to load ktx: invalid or unsupported ktx header in file "<<name<<"\n";
        return false;
    }

    RoxRender::RoxTexture::COLOR_FORMAT cf;

    switch(ktx.pf)
    {
        case RoxFormats::ktx::rgb: cf=RoxRender::RoxTexture::color_rgb; break;
        case RoxFormats::ktx::rgba: cf=RoxRender::RoxTexture::COLOR_RGBA; break;
        case RoxFormats::ktx::bgra: cf=RoxRender::RoxTexture::color_bgra; break;

        case RoxFormats::ktx::etc1: cf=RoxRender::RoxTexture::etc1; break;
        case RoxFormats::ktx::etc2: cf=RoxRender::RoxTexture::etc2; break;
        case RoxFormats::ktx::etc2_eac: cf=RoxRender::RoxTexture::etc2_eac; break;
        case RoxFormats::ktx::etc2_a1: cf=RoxRender::RoxTexture::etc2_a1; break;

        case RoxFormats::ktx::pvr_rgb2b: cf=RoxRender::RoxTexture::pvr_rgb2b; break;
        case RoxFormats::ktx::pvr_rgb4b: cf=RoxRender::RoxTexture::pvr_rgb4b; break;
        case RoxFormats::ktx::pvr_rgba2b: cf=RoxRender::RoxTexture::pvr_rgba2b; break;
        case RoxFormats::ktx::pvr_rgba4b: cf=RoxRender::RoxTexture::pvr_rgba4b; break;

        default: log()<<"unable to load ktx: unsupported color format in file "<<name<<"\n"; return false;
    }

    const int mip_off=m_load_ktx_mip_offset>=int(ktx.mipmap_count)?0:m_load_ktx_mip_offset;
    char *d=(char *)ktx.data;
    RoxMemory::memory_reader r(ktx.data,ktx.data_size);
    for(unsigned int i=0;i<ktx.mipmap_count;++i)
    {
        const unsigned int size=r.read<unsigned int>();
        if(int(i)>=mip_off)
        {
            if(r.get_remained()<size)
            {
                log()<<"unable to load ktx: invalid RoxTexture mipmap size in file "<<name<<"\n";
                return false;
            }

            memmove(d,r.get_data(),size);
            d+=size;
        }
        r.skip(size);
    }

    const int width=ktx.width>>mip_off;
    const int height=ktx.height>>mip_off;
    read_meta(res,data);
    return res.tex.build_texture(ktx.data,width>0?width:1,height>0?height:1,cf,ktx.mipmap_count-mip_off);
}

bool RoxTexture::m_load_dds_flip=false;
int RoxTexture::m_load_dds_mip_offset=0;

bool RoxTexture::load_dds(shared_texture &res,resource_data &data,const char* name)
{
    if(!data.get_size())
        return false;

    if(data.get_size()<4)
        return false;

    if(memcmp(data.get_data(),"DDS ",4)!=0)
        return false;

    RoxFormats::dds dds;
    const size_t header_size=dds.decode_header(data.get_data(),data.get_size());
    if(!header_size)
    {
        log()<<"unable to load dds: invalid or unsupported dds header in file "<<name<<"\n";
        return false;
    }

    if(dds.pf!=RoxFormats::dds::palette8_rgba && dds.pf!=RoxFormats::dds::palette4_rgba) //ToDo
    {
        for(int i=0;i<m_load_dds_mip_offset && dds.mipmap_count > 1;++i)
        {
            dds.data=(char *)dds.data+dds.get_mip_size(0);
            if(dds.width>1)
                dds.width/=2;
            if(dds.height>1)
                dds.height/=2;
            --dds.mipmap_count;
        }
    }

    RoxMemory::RoxTmpBufferRef tmp_buf;

    int mipmap_count=dds.need_generate_mipmaps?-1:dds.mipmap_count;
    RoxRender::RoxTexture::COLOR_FORMAT cf;
    switch(dds.pf)
    {
        case RoxFormats::dds::dxt1: cf=RoxRender::RoxTexture::dxt1; break;
        case RoxFormats::dds::dxt2:
        case RoxFormats::dds::dxt3: cf=RoxRender::RoxTexture::dxt3; break;
        case RoxFormats::dds::dxt4:
        case RoxFormats::dds::dxt5: cf=RoxRender::RoxTexture::dxt5; break;

        case RoxFormats::dds::bgra: cf=RoxRender::RoxTexture::color_bgra; break;
        case RoxFormats::dds::rgba: cf=RoxRender::RoxTexture::COLOR_RGBA; break;
        case RoxFormats::dds::rgb: cf=RoxRender::RoxTexture::color_rgb; break;
        case RoxFormats::dds::greyscale: cf=RoxRender::RoxTexture::greyscale; break;

        case RoxFormats::dds::bgr:
        {
            RoxRender::bitmap_rgb_to_bgr((unsigned char*)dds.data,dds.width,dds.height,3);
            cf=RoxRender::RoxTexture::color_rgb;
        }
        break;

        case RoxFormats::dds::palette8_rgba:
        {
            if(dds.mipmap_count!=1 || dds.type!=RoxFormats::dds::texture_2d) //ToDo
            {
                log()<<"unable to load dds: uncomplete palette8_rgba support, unable to load file "<<name<<"\n";
                return false;
            }

            cf=RoxRender::RoxTexture::COLOR_RGBA;
            dds.data_size=dds.width*dds.height*4;
            tmp_buf.allocate(dds.data_size);
            dds.decode_palette8_rgba(tmp_buf.get_data());
            dds.data=tmp_buf.get_data();
            dds.pf=RoxFormats::dds::bgra;
        }
        break;

        default: log()<<"unable to load dds: unsupported color format in file "<<name<<"\n"; return false;
    }

    bool result=false;

    const bool decode_dxt=cf>=RoxRender::RoxTexture::dxt1 && (!RoxRender::RoxTexture::is_dxt_supported() || dds.height%2>0);

    switch(dds.type)
    {
        case RoxFormats::dds::texture_2d:
        {
            if(decode_dxt)
            {
                tmp_buf.allocate(dds.get_decoded_size());
                dds.decode_dxt(tmp_buf.get_data());
                dds.data_size=tmp_buf.get_size();
                dds.data=tmp_buf.get_data();
                cf=RoxRender::RoxTexture::COLOR_RGBA;
                dds.pf=RoxFormats::dds::bgra;
                if(mipmap_count>1)
                    mipmap_count= -1;
            }

            if(m_load_dds_flip)
            {
                RoxMemory::tmp_buffer_scoped tmp_data(dds.data_size);
                dds.flip_vertical(dds.data,tmp_data.get_data());
                result=res.tex.build_texture(tmp_data.get_data(),dds.width,dds.height,cf,mipmap_count);
            }
            else
                result=res.tex.build_texture(dds.data,dds.width,dds.height,cf,mipmap_count);
        }
        break;

        case RoxFormats::dds::texture_cube:
        {
            if(decode_dxt)
            {
                tmp_buf.allocate(dds.get_decoded_size());
                dds.decode_dxt(tmp_buf.get_data());
                dds.data_size=tmp_buf.get_size();
                dds.data=tmp_buf.get_data();
                cf=RoxRender::RoxTexture::COLOR_RGBA;
                dds.pf=RoxFormats::dds::bgra;
                if(mipmap_count>1)
                    mipmap_count= -1;
            }

            const void *data[6];
            for(int i=0;i<6;++i)
                data[i]=(const char *)dds.data+i*dds.data_size/6;
            result=res.tex.build_cubemap(data,dds.width,dds.height,cf,mipmap_count);
        }
        break;

        default:
        {
            log()<<"unable to load dds: unsupported RoxTexture type in file "<<name<<"\n";
            tmp_buf.free();
            return false;
        }
    }

    tmp_buf.free();
    read_meta(res,data);
    return result;
}

bool RoxTexture::load_tga(shared_texture &res,resource_data &data,const char* name)
{
    if(!data.get_size())
        return false;

    RoxFormats::tga tga;
    const size_t header_size=tga.decode_header(data.get_data(),data.get_size());
    if(!header_size)
        return false;

    RoxRender::RoxTexture::COLOR_FORMAT color_format;
    switch(tga.channels)
    {
        case 4: color_format=RoxRender::RoxTexture::color_bgra; break;
        case 3: color_format=RoxRender::RoxTexture::color_rgb; break;
        case 1: color_format=RoxRender::RoxTexture::greyscale; break;
        default: log()<<"unable to load tga: unsupported color format in file "<<name<<"\n"; return false;
    }

    typedef unsigned char uchar;

    RoxMemory::RoxTmpBufferRef tmp_data;
    const void *color_data=tga.data;
    if(tga.rle)
    {
        tmp_data.allocate(tga.uncompressed_size);
        if(!tga.decode_rle(tmp_data.get_data()))
        {
            tmp_data.free();
            log()<<"unable to load tga: unable to decode rle in file "<<name<<"\n";
            return false;
        }

        color_data=tmp_data.get_data();
    }
    else if(header_size+tga.uncompressed_size>data.get_size())
    {
        log()<<"unable to load tga: lack of data, probably corrupted file "<<name<<"\n";
        return false;
    }

    if(tga.channels==3 || tga.horisontal_flip || tga.vertical_flip)
    {
        if(!tmp_data.get_data())
        {
            tmp_data.allocate(tga.uncompressed_size);

            if(tga.horisontal_flip || tga.vertical_flip)
            {
                if(tga.horisontal_flip)
                    tga.flip_horisontal(color_data,tmp_data.get_data());

                if(tga.vertical_flip)
                    tga.flip_vertical(color_data,tmp_data.get_data());
            }
            else
                tmp_data.copy_from(color_data,tga.uncompressed_size);

            color_data=tmp_data.get_data();
        }
        else
        {
            if(tga.horisontal_flip)
                tga.flip_horisontal(tmp_data.get_data(),tmp_data.get_data());

            if(tga.vertical_flip)
                tga.flip_vertical(tmp_data.get_data(),tmp_data.get_data());
        }

        if(tga.channels==3)
            RoxRender::bitmap_rgb_to_bgr((unsigned char*)color_data,tga.width,tga.height,3);
    }

    const bool result=res.tex.build_texture(color_data,tga.width,tga.height,color_format);
    tmp_data.free();
    read_meta(res,data);
    return result;
}

inline RoxRender::RoxTexture::wrap get_wrap(const std::string &s)
{
    if(s=="repeat")
        return RoxRender::RoxTexture::wrap_repeat;
    if(s=="mirror")
        return RoxRender::RoxTexture::wrap_repeat_mirror;

    return RoxRender::RoxTexture::wrap_clamp;
}

bool RoxTexture::read_meta(shared_texture &res,resource_data &data)
{
    RoxFormats::meta m;
    if(!m.read(data.get_data(),data.get_size()))
        return false;

    bool set_wrap=false;
    RoxRender::RoxTexture::wrap s,t;
    s=t=RoxRender::RoxTexture::wrap_repeat;

    for(int i=0;i<(int)m.values.size();++i)
    {
        if(m.values[i].first=="nya_wrap_s")
            s=get_wrap(m.values[i].second),set_wrap=true;
        else if(m.values[i].first=="nya_wrap_t")
            t=get_wrap(m.values[i].second),set_wrap=true;
        else if(m.values[i].first=="nya_aniso")
        {
            const int aniso=atoi(m.values[i].second.c_str());
            res.tex.set_aniso(aniso>0?aniso:0);
        }
    }

    if(set_wrap)
        res.tex.set_wrap(s,t);

    return true;
}

bool texture_internal::set(int slot) const
{
    if(!m_shared.is_valid())
    {
        RoxRender::RoxTexture::unbind(slot);
        return false;
    }

    m_last_slot=slot;
    m_shared->tex.bind(slot);

    return true;
}

void texture_internal::unset() const
{
    if(!m_shared.is_valid())
        return;

    m_shared->tex.unbind(m_last_slot);
}

unsigned int RoxTexture::get_width() const
{
    if( !internal().get_shared_data().is_valid() )
        return 0;

    return internal().get_shared_data()->tex.get_width();
}

unsigned int RoxTexture::get_height() const
{
    if(!internal().get_shared_data().is_valid())
        return 0;

    return internal().get_shared_data()->tex.get_height();
}

RoxTexture::color_format RoxTexture::get_format() const
{
    if(!internal().get_shared_data().is_valid())
        return RoxRender::RoxTexture::COLOR_RGBA;

    return internal().get_shared_data()->tex.get_color_format();
}

RoxMemory::RoxTmpBufferRef RoxTexture::get_data() const
{
    RoxMemory::RoxTmpBufferRef result;
    if(internal().get_shared_data().is_valid())
        internal().get_shared_data()->tex.get_data(result);
    return result;
}

RoxMemory::RoxTmpBufferRef RoxTexture::get_data(int x,int y,int width,int height) const
{
    RoxMemory::RoxTmpBufferRef result;
    if(internal().get_shared_data().is_valid())
        internal().get_shared_data()->tex.get_data(result,x,y,width,height);
    return result;
}

bool RoxTexture::is_cubemap() const
{
    if(!internal().get_shared_data().is_valid())
        return false;

    return internal().get_shared_data()->tex.is_cubemap();
}

bool RoxTexture::build(const void *data,unsigned int width,unsigned int height,color_format format)
{
    texture_internal::shared_resources::shared_resource_mutable_ref ref;
    if(m_internal.m_shared.get_ref_count()==1 && !m_internal.m_shared.get_name())  //was created and unique
    {
        ref=texture_internal::shared_resources::modify(m_internal.m_shared);
        return ref->tex.build_texture(data,width,height,format);
    }

    m_internal.unload();

    ref=m_internal.get_shared_resources().create();
    if(!ref.is_valid())
        return false;

    m_internal.m_shared=ref;
    return ref->tex.build_texture(data,width,height,format);
}

bool RoxTexture::crop(uint x,uint y,uint width,uint height)
{
    if(!width || !height)
    {
        unload();
        return true;
    }

    if(x==0 && y==0 && width==get_width() && height==get_height())
        return true;

    if(x+width>get_width() || y+height>get_height())
        return false;

    if(!m_internal.m_shared.is_valid())
        return false;

    RoxRender::RoxTexture new_tex;
    if(!new_tex.build_texture(0,width,height,get_format()))
        return false;

    if(!new_tex.copy_region(m_internal.m_shared->tex,x,y,width,height,0,0))
    {
        new_tex.release();

        int channels;
        switch(get_format())
        {
            case RoxRender::RoxTexture::COLOR_RGBA:
            case RoxRender::RoxTexture::color_bgra: channels=4; break;
            case RoxRender::RoxTexture::color_rgb: channels=3; break;
            case RoxRender::RoxTexture::greyscale: channels=1; break;
            default: return false;
        }

        RoxMemory::tmp_buffer_scoped buf(get_data());
        RoxRender::bitmap_crop((unsigned char *)buf.get_data(),get_width(),get_height(),x,y,width,height,channels);
        return build(buf.get_data(),width,height,get_format());
    }

    texture_internal::shared_resources::shared_resource_mutable_ref ref;
    if(m_internal.m_shared.get_ref_count()==1 && !m_internal.m_shared.get_name())  //was created and unique
    {
        ref=texture_internal::shared_resources::modify(m_internal.m_shared);
        ref->tex.release();
        ref->tex=new_tex;
        return true;
    }

    m_internal.unload();

    ref=m_internal.get_shared_resources().create();
    if(!ref.is_valid())
    {
        new_tex.release();
        return false;
    }

    ref->tex=new_tex;
    m_internal.m_shared=ref;

    return true;
}

inline void grey_to_color(const unsigned char *data_from,unsigned char *data_to,size_t src_size,int channels)
{
    const unsigned char *src=data_from;
    unsigned char *dst=data_to;

    for(size_t i=0;i<src_size;++i, ++src)
    {
        for(int j=0;j<channels;++j, ++dst)
            *dst=*src;
    }
}

bool RoxTexture::update_region(const void *data,uint x,uint y,uint width,uint height,color_format format,int mip)
{
    if(!data)
        return false;

    if(get_format()==format)
        return update_region(data,x,y,width,height,mip);

    switch(get_format())
    {
        case RoxRender::RoxTexture::color_bgra:
        case RoxRender::RoxTexture::COLOR_RGBA:
        {
            RoxMemory::tmp_buffer_scoped buf(width*height*4);

            if(format==RoxRender::RoxTexture::color_bgra || format==RoxRender::RoxTexture::COLOR_RGBA)
            {
                buf.copy_from(data,buf.get_size());
                RoxRender::bitmap_rgb_to_bgr((unsigned char *)buf.get_data(),width,height,4);
            }
            else if(format==RoxRender::RoxTexture::color_rgb)
            {
                RoxRender::bitmap_rgb_to_rgba((const unsigned char *)data,width,height,255,(unsigned char *)buf.get_data());
                if(get_format()==RoxRender::RoxTexture::color_bgra)
                    RoxRender::bitmap_rgb_to_bgr((unsigned char *)buf.get_data(),width,height,4);
            }
            else if(format==RoxRender::RoxTexture::greyscale)
                grey_to_color((const unsigned char *)data,(unsigned char *)buf.get_data(),width*height,4);
            else
                return false;

            return update_region(buf.get_data(),x,y,width,height,mip);
        }
        break;

        case RoxRender::RoxTexture::color_rgb:
        {
            RoxMemory::tmp_buffer_scoped buf(width*height*3);
            if(format==RoxRender::RoxTexture::color_bgra || format==RoxRender::RoxTexture::COLOR_RGBA)
            {
                RoxRender::bitmap_rgba_to_rgb((const unsigned char *)data,width,height,(unsigned char *)buf.get_data());
                if(format==RoxRender::RoxTexture::color_bgra)
                    RoxRender::bitmap_rgb_to_bgr((unsigned char *)buf.get_data(),width,height,3);
            }
            else if(format==RoxRender::RoxTexture::greyscale)
                grey_to_color((const unsigned char *)data,(unsigned char *)buf.get_data(),width*height,3);
            else
                return false;

            return update_region(buf.get_data(),x,y,width,height,mip);
        }
        break;

        //ToDo

        default: return false;
    }

    return false;
}

bool RoxTexture::update_region(const void *data,uint x,uint y,uint width,uint height,int mip)
{
    if(!width || !height)
        return x<=get_width() && y<=get_height();

    texture_internal::shared_resources::shared_resource_mutable_ref ref;
    if(m_internal.m_shared.get_ref_count()==1 && !m_internal.m_shared.get_name())  //was created and unique
    {
        ref=texture_internal::shared_resources::modify(m_internal.m_shared);
        return ref->tex.update_region(data,x,y,width,height);
    }

    if(!m_internal.m_shared.is_valid())
        return false;

    const uint w=m_internal.m_shared->tex.get_width();
    const uint h=m_internal.m_shared->tex.get_height();

    if(!x && !y && w==width && h==height && mip<0)
        return build(data,w,h,get_format());

    shared_texture new_stex;
    if(!new_stex.tex.build_texture(0,w,h,get_format()))
        return false;

    if(!new_stex.tex.copy_region(m_internal.m_shared->tex,0,0,w,h,0,0))
    {
        const RoxMemory::tmp_buffer_scoped buf(get_data(0,0,w,h));
        if(!buf.get_size() || !new_stex.tex.build_texture(buf.get_data(),w,h,get_format()))
        {
            new_stex.tex.release();
            return false;
        }
    }

    create(new_stex);
    return update_region(data,x,y,width,height,mip);
}

bool RoxTexture::update_region(const texture_proxy &source,uint x,uint y)
{
    if(!source.is_valid())
        return false;

    return update_region(source,0,0,source->get_width(),source->get_height(),x,y);
}

bool RoxTexture::update_region(const RoxTexture &source,unsigned int x,unsigned int y)
{
    return update_region(texture_proxy(source),x,y);
}

bool RoxTexture::update_region(const texture_proxy &source,uint src_x,uint src_y,uint width,uint height,uint dst_x,uint dst_y)
{
    if(!source.is_valid() || !source->internal().get_shared_data().is_valid())
        return false;

    if(src_x+width>source->get_width() || src_y+height>source->get_height())
        return false;

    if(dst_x+width>get_width() || dst_y+height>get_height())
        return false;

    if(!internal().get_shared_data().is_valid())
        return false;

    if(m_internal.m_shared.get_ref_count()==1 && !m_internal.m_shared.get_name())  //was created and unique
    {
        RoxRender::RoxTexture &tex=texture_internal::shared_resources::modify(m_internal.m_shared)->tex;
        if(tex.copy_region(source->internal().get_shared_data()->tex,src_x,src_y,width,height,dst_x,dst_y))
            return true;
    }

    const RoxMemory::tmp_buffer_scoped buf(source->get_data(src_x,src_y,width,height));
    return update_region(buf.get_data(),dst_x,dst_y,width,height,source->get_format());
}

bool RoxTexture::update_region(const RoxTexture &source,uint src_x,uint src_y,uint width,uint height,uint dst_x,uint dst_y)
{
    return update_region(texture_proxy(source),src_x,src_y,width,height,dst_x,dst_y);
}

void RoxTexture::set_resources_prefix(const char *prefix)
{
    texture_internal::set_resources_prefix(prefix);
}

void RoxTexture::register_load_function(texture_internal::load_function function,bool clear_default)
{
    texture_internal::register_load_function(function,clear_default);
}

}
