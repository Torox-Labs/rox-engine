//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "texture.h"
#include "RoxScene.h"
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
#include <cstdlib>

namespace RoxScene
{

int texture::m_load_ktx_mip_offset=0;

bool texture::load_ktx(shared_texture &res,resource_data &data,const char* name)
{
    if(!data.getSize())
        return false;

    if(data.getSize()<12)
        return false;

    if(memcmp((const char *)data.getData()+1,"KTX ",4)!=0)
        return false;

    RoxFormats::KhronosTexture ktx;
    const size_t header_size=ktx.decodeHeader(data.getData(),data.getSize());
    if(!header_size)
    {
        log()<<"unable to load ktx: invalid or unsupported ktx header in file "<<name<<"\n";
        return false;
    }

    RoxRender::RoxTexture::COLOR_FORMAT cf;

    switch(ktx.pf)
    {
        case RoxFormats::KhronosTexture::RGB: cf=RoxRender::RoxTexture::COLOR_RGB; break;
        case RoxFormats::KhronosTexture::RGBA: cf=RoxRender::RoxTexture::COLOR_RGBA; break;
        case RoxFormats::KhronosTexture::BGRA: cf=RoxRender::RoxTexture::COLOR_RGBA; break;

        case RoxFormats::KhronosTexture::ETC1: cf=RoxRender::RoxTexture::ETC1; break;
        case RoxFormats::KhronosTexture::ETC2: cf=RoxRender::RoxTexture::ETC2; break;
        case RoxFormats::KhronosTexture::ETC2_EAC: cf=RoxRender::RoxTexture::ETC2_EAC; break;
        case RoxFormats::KhronosTexture::ETC2_A1: cf=RoxRender::RoxTexture::ETC2_A1; break;

        case RoxFormats::KhronosTexture::PVR_RGB2B: cf=RoxRender::RoxTexture::PVR_RGB2B; break;
        case RoxFormats::KhronosTexture::PVR_RGB4B: cf=RoxRender::RoxTexture::PVR_RGB4B; break;
        case RoxFormats::KhronosTexture::PVR_RGBA2B: cf=RoxRender::RoxTexture::PVR_RGBA2B; break;
        case RoxFormats::KhronosTexture::PVR_RGBA4B: cf=RoxRender::RoxTexture::PVR_RGBA4B; break;

        default: log()<<"unable to load ktx: unsupported color format in file "<<name<<"\n"; return false;
    }

    const int mip_off=m_load_ktx_mip_offset>=int(ktx.mipmap_count)?0:m_load_ktx_mip_offset;
    char *d=(char *)ktx.data;
    RoxMemory::RoxMemoryReader r(ktx.data,ktx.data_size);
    for(unsigned int i=0;i<ktx.mipmap_count;++i)
    {
        const unsigned int size=r.read<unsigned int>();
        if(int(i)>=mip_off)
        {
            if(r.getRemained()<size)
            {
                log()<<"unable to load ktx: invalid RoxTexture mipmap size in file "<<name<<"\n";
                return false;
            }

            memmove(d,r.getData(),size);
            d+=size;
        }
        r.skip(size);
    }

    const int width=ktx.width>>mip_off;
    const int height=ktx.height>>mip_off;
    read_meta(res,data);
    return res.tex.buildTexture(ktx.data,width>0?width:1,height>0?height:1,cf,ktx.mipmap_count-mip_off);
}

bool texture::m_load_dds_flip=false;
int texture::m_load_dds_mip_offset=0;

bool texture::load_dds(shared_texture &res,resource_data &data,const char* name)
{
    if(!data.getSize())
        return false;

    if(data.getSize()<4)
        return false;

    if(memcmp(data.getData(),"DDS ",4)!=0)
        return false;

    RoxFormats::DirectDrawSurface dds;
    const size_t header_size=dds.decodeHeader(data.getData(),data.getSize());
    if(!header_size)
    {
        log()<<"unable to load dds: invalid or unsupported dds header in file "<<name<<"\n";
        return false;
    }

    if(dds.pf!=RoxFormats::DirectDrawSurface::PALETTE8_RGBA && dds.pf!=RoxFormats::DirectDrawSurface::PALETTE4_RGBA) //ToDo
    {
        for(int i=0;i<m_load_dds_mip_offset && dds.mipmap_count > 1;++i)
        {
            dds.data=(char *)dds.data+dds.getMipSize(0);
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
        case RoxFormats::DirectDrawSurface::DXT1: cf=RoxRender::RoxTexture::DXT1; break;
        case RoxFormats::DirectDrawSurface::DXT2:
        case RoxFormats::DirectDrawSurface::DXT3: cf=RoxRender::RoxTexture::DXT3; break;
        case RoxFormats::DirectDrawSurface::DXT4:
        case RoxFormats::DirectDrawSurface::DXT5: cf=RoxRender::RoxTexture::DXT5; break;

        case RoxFormats::DirectDrawSurface::BGRA: cf=RoxRender::RoxTexture::COLOR_RGBA; break;
        case RoxFormats::DirectDrawSurface::RGBA: cf=RoxRender::RoxTexture::COLOR_RGBA; break;
        case RoxFormats::DirectDrawSurface::RGB: cf=RoxRender::RoxTexture::COLOR_RGB; break;
        case RoxFormats::DirectDrawSurface::GREYSCALE: cf=RoxRender::RoxTexture::GREYSCALE; break;

        case RoxFormats::DirectDrawSurface::BGR:
        {
            RoxRender::bitmapRgbToBgr((unsigned char*)dds.data,dds.width,dds.height,3);
            cf=RoxRender::RoxTexture::COLOR_RGB;
        }
        break;

        case RoxFormats::DirectDrawSurface::PALETTE8_RGBA:
        {
            if(dds.mipmap_count!=1 || dds.type!=RoxFormats::DirectDrawSurface::TEXTURE_2D) //ToDo
            {
                log()<<"unable to load dds: uncomplete palette8_rgba support, unable to load file "<<name<<"\n";
                return false;
            }

            cf=RoxRender::RoxTexture::COLOR_RGBA;
            dds.data_size=dds.width*dds.height*4;
            tmp_buf.allocate(dds.data_size);
            dds.decodePalette8Rgba(tmp_buf.getData());
            dds.data=tmp_buf.getData();
            dds.pf=RoxFormats::DirectDrawSurface::BGRA;
        }
        break;

        default: log()<<"unable to load dds: unsupported color format in file "<<name<<"\n"; return false;
    }

    bool result=false;

    const bool decode_dxt=cf>=RoxRender::RoxTexture::DXT1 && (!RoxRender::RoxTexture::isDxtSupported() || dds.height%2>0);

    switch(dds.type)
    {
        case RoxFormats::DirectDrawSurface::TEXTURE_2D:
        {
            if(decode_dxt)
            {
                tmp_buf.allocate(dds.getDecodedSize());
                dds.decodeDxt(tmp_buf.getData());
                dds.data_size=tmp_buf.getSize();
                dds.data=tmp_buf.getData();
                cf=RoxRender::RoxTexture::COLOR_RGBA;
                dds.pf=RoxFormats::DirectDrawSurface::BGRA;
                if(mipmap_count>1)
                    mipmap_count= -1;
            }

            if(m_load_dds_flip)
            {
                RoxMemory::RoxTmpBufferScoped tmp_data(dds.data_size);
                dds.flipVertical(dds.data,tmp_data.getData());
                result=res.tex.buildTexture(tmp_data.getData(),dds.width,dds.height,cf,mipmap_count);
            }
            else
                result=res.tex.buildTexture(dds.data,dds.width,dds.height,cf,mipmap_count);
        }
        break;

        case RoxFormats::DirectDrawSurface::TEXTURE_CUBE:
        {
            if(decode_dxt)
            {
                tmp_buf.allocate(dds.getDecodedSize());
                dds.decodeDxt(tmp_buf.getData());
                dds.data_size=tmp_buf.getSize();
                dds.data=tmp_buf.getData();
                cf=RoxRender::RoxTexture::COLOR_RGBA;
                dds.pf=RoxFormats::DirectDrawSurface::BGRA;
                if(mipmap_count>1)
                    mipmap_count= -1;
            }

            const void *data[6];
            for(int i=0;i<6;++i)
                data[i]=(const char *)dds.data+i*dds.data_size/6;
            result=res.tex.buildCubemap(data,dds.width,dds.height,cf,mipmap_count);
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

bool texture::load_tga(shared_texture &res,resource_data &data,const char* name)
{
    if(!data.getSize())
        return false;

    RoxFormats::RTga tga;
    const size_t header_size=tga.decodeHeader(data.getData(),data.getSize());
    if(!header_size)
        return false;

    RoxRender::RoxTexture::COLOR_FORMAT color_format;
    switch(tga.channels)
    {
        case 4: color_format=RoxRender::RoxTexture::COLOR_RGBA; break;
        case 3: color_format=RoxRender::RoxTexture::COLOR_RGB; break;
        case 1: color_format=RoxRender::RoxTexture::GREYSCALE; break;
        default: log()<<"unable to load tga: unsupported color format in file "<<name<<"\n"; return false;
    }

    typedef unsigned char uchar;

    RoxMemory::RoxTmpBufferRef tmp_data;
    const void *color_data=tga.data;
    if(tga.rle)
    {
        tmp_data.allocate(tga.uncompressedSize);
        if(!tga.decodeRle(tmp_data.getData()))
        {
            tmp_data.free();
            log()<<"unable to load tga: unable to decode rle in file "<<name<<"\n";
            return false;
        }

        color_data=tmp_data.getData();
    }
    else if(header_size+tga.uncompressedSize>data.getSize())
    {
        log()<<"unable to load tga: lack of data, probably corrupted file "<<name<<"\n";
        return false;
    }

    if(tga.channels==3 || tga.horisontalFlip || tga.verticalFlip)
    {
        if(!tmp_data.getData())
        {
            tmp_data.allocate(tga.uncompressedSize);

            if(tga.horisontalFlip || tga.verticalFlip)
            {
                if(tga.horisontalFlip)
                    tga.flipHorisontal(color_data,tmp_data.getData());

                if(tga.verticalFlip)
                    tga.flipVertical(color_data,tmp_data.getData());
            }
            else
                tmp_data.copyFrom(color_data,tga.uncompressedSize);

            color_data=tmp_data.getData();
        }
        else
        {
            if(tga.horisontalFlip)
                tga.flipHorisontal(tmp_data.getData(),tmp_data.getData());

            if(tga.verticalFlip)
                tga.flipVertical(tmp_data.getData(),tmp_data.getData());
        }

        if(tga.channels==3)
            RoxRender::bitmapRgbToBgr((unsigned char*)color_data,tga.width,tga.height,3);
    }

    const bool result=res.tex.buildTexture(color_data,tga.width,tga.height,color_format);
    tmp_data.free();
    read_meta(res,data);
    return result;
}

inline RoxRender::RoxTexture::WRAP get_wrap(const std::string &s)
{
    if(s=="repeat")
        return RoxRender::RoxTexture::WRAP_REPEAT;
    if(s=="mirror")
        return RoxRender::RoxTexture::WRAP_REPEAT_MIRROR;

    return RoxRender::RoxTexture::WRAP_CLAMP;
}

bool read_meta(shared_texture &res,resource_data &data)
{
    RoxFormats::Meta m;
    if(!m.read(data.getData(),data.getSize()))
        return false;

    bool set_wrap=false;
    RoxRender::RoxTexture::WRAP s,t;
    s=t=RoxRender::RoxTexture::WRAP_REPEAT;

    for(int i=0;i<(int)m.values.size();++i)
    {
        if(m.values[i].first=="nya_wrap_s")
            s=get_wrap(m.values[i].second),set_wrap=true;
        else if(m.values[i].first=="nya_wrap_t")
            t=get_wrap(m.values[i].second),set_wrap=true;
        else if(m.values[i].first=="nya_aniso")
        {
            const int aniso=atoi(m.values[i].second.c_str());
            res.tex.setAniso(aniso>0?aniso:0);
        }
    }

    if(set_wrap)
        res.tex.setWrap(s,t);

    return true;
}

bool texture_internal::set(int slot) const
{
    if(!m_shared.isValid())
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
    if(!m_shared.isValid())
        return;

    m_shared->tex.unbind(m_last_slot);
}

unsigned int texture::get_width() const
{
    if( !internal().get_shared_data().isValid() )
        return 0;

    return internal().get_shared_data()->tex.getWidth();
}

unsigned int texture::get_height() const
{
    if(!internal().get_shared_data().isValid())
        return 0;

    return internal().get_shared_data()->tex.getHeight();
}

RoxRender::RoxTexture::COLOR_FORMAT texture::get_format() const
{
    if(!internal().get_shared_data().isValid())
        return RoxRender::RoxTexture::COLOR_RGBA;

    return internal().get_shared_data()->tex.getColorFormat();
}

RoxMemory::RoxTmpBufferRef texture::get_data() const
{
    RoxMemory::RoxTmpBufferRef result;
    if(internal().get_shared_data().isValid())
        internal().get_shared_data()->tex.getData(result);
    return result;
}

RoxMemory::RoxTmpBufferRef texture::get_data(int x,int y,int width,int height) const
{
    RoxMemory::RoxTmpBufferRef result;
    if(internal().get_shared_data().isValid())
        internal().get_shared_data()->tex.getData(result,x,y,width,height);
    return result;
}

bool texture::is_cubemap() const
{
    if(!internal().get_shared_data().isValid())
        return false;

    return internal().get_shared_data()->tex.isCubemap();
}

bool texture::build(const void *data,unsigned int width,unsigned int height,color_format format)
{
    texture_internal::shared_resources::RoxSharedResourceMutableRef ref;
    if(m_internal.m_shared.getRefCount()==1 && !m_internal.m_shared.getName())  //was created and unique
    {
        ref=texture_internal::shared_resources::modify(m_internal.m_shared);
        return ref->tex.buildTexture(data,width,height,format);
    }

    m_internal.unload();

    ref=m_internal.get_shared_resources().create();
    if(!ref.isValid())
        return false;

    m_internal.m_shared=ref;
    return ref->tex.buildTexture(data,width,height,format);
}

bool texture::crop(uint x,uint y,uint width,uint height)
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

    if(!m_internal.m_shared.isValid())
        return false;

    RoxRender::RoxTexture new_tex;
    if(!new_tex.buildTexture(0,width,height,get_format()))
        return false;

    if(!new_tex.copyRegion(m_internal.m_shared->tex,x,y,width,height,0,0))
    {
        new_tex.release();

        int channels;
        switch(get_format())
        {
            case RoxRender::RoxTexture::COLOR_RGBA:
            case RoxRender::RoxTexture::COLOR_BGRA: channels=4; break;
            case RoxRender::RoxTexture::COLOR_RGB: channels=3; break;
            case RoxRender::RoxTexture::GREYSCALE: channels=1; break;
            default: return false;
        }

        RoxMemory::RoxTmpBufferScoped buf(get_data());
        RoxRender::bitmapCrop((unsigned char *)buf.getData(),get_width(),get_height(),x,y,width,height,channels);
        return build(buf.getData(),width,height,get_format());
    }

    texture_internal::shared_resources::RoxSharedResourceMutableRef ref;
    if(m_internal.m_shared.getRefCount()==1 && !m_internal.m_shared.getName())  //was created and unique
    {
        ref=texture_internal::shared_resources::modify(m_internal.m_shared);
        ref->tex.release();
        ref->tex=new_tex;
        return true;
    }

    m_internal.unload();

    ref=m_internal.get_shared_resources().create();
    if(!ref.isValid())
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

bool texture::update_region(const void *data,uint x,uint y,uint width,uint height,color_format format,int mip)
{
    if(!data)
        return false;

    if(get_format()==format)
        return update_region(data,x,y,width,height,mip);

    switch(get_format())
    {
        case RoxRender::RoxTexture::COLOR_RGBA:
        case RoxRender::RoxTexture::COLOR_BGRA:
        {
            RoxMemory::RoxTmpBufferScoped buf(width*height*4);

            if(format==RoxRender::RoxTexture::COLOR_RGBA || format==RoxRender::RoxTexture::COLOR_RGBA)
            {
                buf.copyFrom(data,buf.getSize());
                RoxRender::bitmapRgbToBgr((unsigned char *)buf.getData(),width,height,4);
            }
            else if(format==RoxRender::RoxTexture::COLOR_RGB)
            {
                RoxRender::bitmapRgbToRgba((const unsigned char *)data,width,height,255,(unsigned char *)buf.getData());
                if(get_format()==RoxRender::RoxTexture::COLOR_RGBA)
                    RoxRender::bitmapRgbToBgr((unsigned char *)buf.getData(),width,height,4);
            }
            else if(format==RoxRender::RoxTexture::GREYSCALE)
                grey_to_color((const unsigned char *)data,(unsigned char *)buf.getData(),width*height,4);
            else
                return false;

            return update_region(buf.getData(),x,y,width,height,mip);
        }
        break;

        case RoxRender::RoxTexture::COLOR_RGB:
        {
            RoxMemory::RoxTmpBufferScoped buf(width*height*3);
            if(format==RoxRender::RoxTexture::COLOR_RGBA || format==RoxRender::RoxTexture::COLOR_RGBA)
            {
                RoxRender::bitmapRgbaToRgb((const unsigned char *)data,width,height,(unsigned char *)buf.getData());
                if(format==RoxRender::RoxTexture::COLOR_RGBA)
                    RoxRender::bitmapRgbToBgr((unsigned char *)buf.getData(),width,height,3);
            }
            else if(format==RoxRender::RoxTexture::GREYSCALE)
                grey_to_color((const unsigned char *)data,(unsigned char *)buf.getData(),width*height,3);
            else
                return false;

            return update_region(buf.getData(),x,y,width,height,mip);
        }
        break;

        //ToDo

        default: return false;
    }

    return false;
}

bool texture::update_region(const void *data,uint x,uint y,uint width,uint height,int mip)
{
    if(!width || !height)
        return x<=get_width() && y<=get_height();

    texture_internal::shared_resources::RoxSharedResourceMutableRef ref;
    if(m_internal.m_shared.getRefCount()==1 && !m_internal.m_shared.getName())  //was created and unique
    {
        ref=texture_internal::shared_resources::modify(m_internal.m_shared);
        return ref->tex.updateRegion(data,x,y,width,height);
    }

    if(!m_internal.m_shared.isValid())
        return false;

    const uint w=m_internal.m_shared->tex.getWidth();
    const uint h=m_internal.m_shared->tex.getHeight();

    if(!x && !y && w==width && h==height && mip<0)
        return build(data,w,h,get_format());

    shared_texture new_stex;
    if(!new_stex.tex.buildTexture(0,w,h,get_format()))
        return false;

    if(!new_stex.tex.copyRegion(m_internal.m_shared->tex,0,0,w,h,0,0))
    {
        const RoxMemory::RoxTmpBufferScoped buf(get_data(0,0,w,h));
        if(!buf.getSize() || !new_stex.tex.buildTexture(buf.getData(),w,h,get_format()))
        {
            new_stex.tex.release();
            return false;
        }
    }

    create(new_stex);
    return update_region(data,x,y,width,height,mip);
}

bool texture::update_region(const texture_proxy &source,uint x,uint y)
{
    if(!source.isValid())
        return false;

    return update_region(source,0,0,source->get_width(),source->get_height(),x,y);
}

bool texture::update_region(const texture &source,unsigned int x,unsigned int y)
{
    return update_region(texture_proxy(source),x,y);
}

bool texture::update_region(const texture_proxy &source,uint src_x,uint src_y,uint width,uint height,uint dst_x,uint dst_y)
{
    if(!source.isValid() || !source->internal().get_shared_data().isValid())
        return false;

    if(src_x+width>source->get_width() || src_y+height>source->get_height())
        return false;

    if(dst_x+width>get_width() || dst_y+height>get_height())
        return false;

    if(!internal().get_shared_data().isValid())
        return false;

    if(m_internal.m_shared.getRefCount()==1 && !m_internal.m_shared.getName())  //was created and unique
    {
        RoxRender::RoxTexture &tex=texture_internal::shared_resources::modify(m_internal.m_shared)->tex;
        if(tex.copyRegion(source->internal().get_shared_data()->tex,src_x,src_y,width,height,dst_x,dst_y))
            return true;
    }

    const RoxMemory::RoxTmpBufferScoped buf(source->get_data(src_x,src_y,width,height));
    return update_region(buf.getData(),dst_x,dst_y,width,height,source->get_format());
}

bool texture::update_region(const texture &source,uint src_x,uint src_y,uint width,uint height,uint dst_x,uint dst_y)
{
    return update_region(texture_proxy(source),src_x,src_y,width,height,dst_x,dst_y);
}

void texture::set_resources_prefix(const char *prefix)
{
    texture_internal::set_resources_prefix(prefix);
}

void texture::register_load_function(texture_internal::load_function function,bool clear_default)
{
    texture_internal::register_load_function(function,clear_default);
}

}
