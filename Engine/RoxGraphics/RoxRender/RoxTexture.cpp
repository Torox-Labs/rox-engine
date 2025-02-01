// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxTexture.h"
#include "RoxBitmap.h"
#include "RoxRenderOpengl.h"
#include "RoxMemory/RoxTmpBuffers.h"

namespace RoxRender
{

namespace
{
    RoxTexture::WRAP default_wrap_s=RoxTexture::WRAP_REPEAT;
    RoxTexture::WRAP default_wrap_t=RoxTexture::WRAP_REPEAT;
    RoxTexture::FILTER default_min_filter=RoxTexture::FILTER_LINEAR;
    RoxTexture::FILTER default_mag_filter=RoxTexture::FILTER_LINEAR;
    RoxTexture::FILTER default_mip_filter=RoxTexture::FILTER_LINEAR;
    unsigned int default_aniso=0;
}

bool RoxTexture::buildTexture(const void *data_a[6],bool is_cubemap,unsigned int width,unsigned int height,
                            COLOR_FORMAT format,int mip_count)
{
    if(width==0 || height==0)
    {
        log()<<"Unable to build RoxTexture: invalid width or height\n";
        release();
        return false;
    }

    if(width>getMaxDimension() || height> getMaxDimension())
    {
        log()<<"Unable to build RoxTexture: width or height is too high, maximum is "<<getMaxDimension()<<"\n";
        release();
        return false;
    }

    if(is_cubemap && width!=height)
    {
        release();
        return false;
    }

    const void *data=data_a?data_a[0]:0;

    if(format>=DXT1)
    {
        if(!data || mip_count==0)
        {
            log()<<"Unable to build RoxTexture: compressed format with invalid data\n";
            release();
            return false;
        }
    }

    if(format>=COLOR_R32F && mip_count<0)
        mip_count=1;

    const bool is_pvrtc=format==PVR_RGB2B || format==PVR_RGBA2B || format==PVR_RGB4B || format==PVR_RGBA4B;
    const bool pot=((width&(width-1))==0 && (height&(height-1))==0);

    if(is_pvrtc && !(width==height && pot))
    {
        log()<<"Unable to build RoxTexture: pvr compression supports only square pot textures\n";
        release();
        return false;
    }

    if(!is_cubemap && !m_is_cubemap && m_width==width && m_height==height && m_format==format && data)
    {
        getApiInterface().updateTexture(m_tex,data,0,0,width,height,-1);
        return true;
    }
    else
        release();

    if(!data)
        mip_count=0;
    else if(!pot)
        mip_count=1;

    RoxMemory::RoxTmpBufferRef tmp_buf;

    if(!getApiInterface().isTextureFormatSupported(format))
    {
        if(format==DEPTH24)
        {
            if(data)
                return false; //ToDo

            format=DEPTH32;
        }
        else if(format==COLOR_RGBA || format==COLOR_RGB)
        {
            if(!getApiInterface().isTextureFormatSupported(COLOR_BGRA))
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
                    unsigned char *to=(unsigned char *)tmp_buf.getData(i*size);
                    (is_cubemap?data_a[i]:data)=to;
                    if(format==COLOR_RGB)
                    {
                        for(size_t j=0;j<size;j+=4,from+=3,to+=4)
                        {
                            memcpy(to,from,3);
                            to[3]=255;
                        }
                    }
                    else if(format==COLOR_BGRA)
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
            format=COLOR_BGRA;
        }
        else
        {
            log()<<"Unable to build RoxTexture: unsupported format\n";
            return false;
        }
    }

    if(is_cubemap)
        m_tex=getApiInterface().createCubemap(data_a,width,format,mip_count);
    else
        m_tex=getApiInterface().createTexture(data,width,height,format,mip_count);

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
    getApiInterface().setTextureFilter(m_tex,m_filter_min,m_filter_mag,m_filter_mip,m_aniso);

    if(!is_cubemap)
    {
        const bool force_clamp=!pot && !isPlatformRestrictionsIgnored();
        getApiInterface().setTextureWrap(m_tex,force_clamp? WRAP_CLAMP:default_wrap_s,force_clamp?WRAP_CLAMP:default_wrap_t);
    }
    return true;
}

bool RoxTexture::buildTexture(const void *data,unsigned int width,unsigned int height,COLOR_FORMAT format,int mip_count)
{
    const void *data_a[6]={data};
    return buildTexture(data_a,false,width,height,format,mip_count);
}

bool RoxTexture::buildCubemap(const void *data[6],unsigned int width,unsigned int height,COLOR_FORMAT format,int mip_count)
{
    return buildTexture(data,true,width,height,format,mip_count);
}

bool RoxTexture::updateRegion(const void *data,unsigned int x,unsigned int y,unsigned int width,unsigned int height,int mip)
{
    if(m_tex<0 && m_is_cubemap)
        return false;

    unsigned int mip_width=mip<0?m_width:m_width>>mip, mip_height=mip<0?m_height:m_height>>mip;
    if(!mip_width) mip_width=1;
    if(!mip_height) mip_height=1;

    if(x+width>mip_width || y+height>mip_height)
        return false;

    if(m_format>=DEPTH16)
        return false;

    getApiInterface().updateTexture(m_tex,data,x,y,width,height,-1);
    return true;
}

bool RoxTexture::copyRegion(const RoxTexture &src,uint src_x,uint src_y,uint width,uint height,uint dst_x,uint dst_y)
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

void RoxTexture::bind(unsigned int layer) const
{
    if(layer>=IRoxRenderApi::State::max_layers)
        return;

    getApiState().textures[layer]=m_tex;
}

void RoxTexture::unbind(unsigned int layer)
{
    if(layer>=IRoxRenderApi::State::max_layers)
        return;

    getApiState().textures[layer]=-1;
}

bool RoxTexture::getData(RoxMemory::RoxTmpBufferRef &data) const
{
    return getData(data,0,0,m_width,m_height);
}

bool RoxTexture::getData(RoxMemory::RoxTmpBufferRef&data,unsigned int x,unsigned int y,unsigned int w,unsigned int h) const
{
    if(m_tex<0 || !w || !h || x+w>m_width || y+h>m_height)
        return false;

    size_t size=w*h;
    switch(m_format)
    {
        case GREYSCALE: break;
        case COLOR_RGB: size*=3; break;

        case COLOR_R32F: break;
        case COLOR_RGB32F: size*=3*sizeof(float); break;
        case COLOR_RGBA32F: size*=4*sizeof(float); break;

        case DEPTH16: break; size*=2; break;
        case DEPTH24: break; size*=3; break;

        default: size*=4;
    }

    data.allocate(size);
    if(!getApiInterface().getTextureData(m_tex,x,y,w,h,data.getData()))
    {
        data.free();
        return false;
    }
    return true;
}

unsigned int RoxTexture::getWidth() const { return m_width; }
unsigned int RoxTexture::getHeight() const { return m_height; }
RoxTexture::COLOR_FORMAT RoxTexture::getColorFormat() const { return m_format; }
bool RoxTexture::isCubemap() const { return m_is_cubemap; }

void RoxTexture::setWrap(WRAP s,WRAP t)
{
    if(m_tex<0 || m_is_cubemap)
        return;

    const bool pot=((m_width&(m_width-1))==0 && (m_height&(m_height-1))==0);
    if(!pot)
        s=t=WRAP_CLAMP;

    getApiInterface().setTextureWrap(m_tex,s,t);
}

void RoxTexture::setAniso(unsigned int level)
{
    m_aniso=level;
    m_aniso_set=true;

    if(m_tex<0)
        return;

    getApiInterface().setTextureFilter(m_tex,m_filter_min,m_filter_mag,m_filter_mip,m_aniso);
}

void RoxTexture::setFilter(FILTER minification,FILTER magnification,FILTER mipmap)
{
    m_filter_min=minification;
    m_filter_mag=magnification;
    m_filter_mip=mipmap;
    m_filter_set=true;

    if(m_tex<0)
        return;

    getApiInterface().setTextureFilter(m_tex,m_filter_min,m_filter_mag,m_filter_mip,m_aniso);
}

void RoxTexture::setDefaultWrap(WRAP s,WRAP t)
{
    default_wrap_s=s;
    default_wrap_t=t;
}

void RoxTexture::setDefaultFilter(FILTER minification,FILTER magnification,FILTER mipmap)
{
    default_min_filter=minification;
    default_mag_filter=magnification;
    default_mip_filter=mipmap;
}

void RoxTexture::setDefaultAniso(unsigned int level) { default_aniso=level; }

void RoxTexture::getDefaultWrap(WRAP &s,WRAP &t)
{
    s=default_wrap_s;
    t=default_wrap_t;
}

void RoxTexture::getDefaultFilter(FILTER &minification,FILTER &magnification,FILTER &mipmap)
{
    minification=default_min_filter;
    magnification=default_mag_filter;
    mipmap=default_mip_filter;
}

unsigned int RoxTexture::getDefaultAniso() { return default_aniso; }
unsigned int RoxTexture::getMaxDimension() { return getApiInterface().getMaxTextureDimension(); }
bool RoxTexture::isDxtSupported() { return getApiInterface().isTextureFormatSupported(DXT1); }

unsigned int RoxTexture::getFormatBpp(RoxTexture::COLOR_FORMAT format)
{
    switch(format)
    {
        case RoxTexture::COLOR_BGRA: return 32;
        case RoxTexture::COLOR_RGBA: return 32;
        case RoxTexture::COLOR_RGB: return 24;
        case RoxTexture::GREYSCALE: return 8;
        case RoxTexture::COLOR_RGBA32F: return 32*4;
        case RoxTexture::COLOR_RGB32F: return 32*3;
        case RoxTexture::COLOR_R32F: return 32;
        case RoxTexture::DEPTH16: return 16;
        case RoxTexture::DEPTH24: return 24;
        case RoxTexture::DEPTH32: return 32;

        case RoxTexture::DXT1: return 4;
        case RoxTexture::DXT3: return 8;
        case RoxTexture::DXT5: return 8;

        case RoxTexture::ETC1: return 4;
        case RoxTexture::ETC2: return 4;
        case RoxTexture::ETC2_A1: return 4;
        case RoxTexture::ETC2_EAC: return 8;

        case RoxTexture::PVR_RGB2B: return 2;
        case RoxTexture::PVR_RGB4B: return 4;
        case RoxTexture::PVR_RGBA2B: return 2;
        case RoxTexture::PVR_RGBA4B: return 4;
    };

    return 0;
}

ID3D11Texture2D *RoxTexture::getDx11TexId() const
{
    //renderapi

    return 0;
}

unsigned int RoxTexture::getGlTexId() const
{
    if(m_tex<0)
        return 0;

    if(getRenderApi()==RENDER_API_OPENGL)
        return RoxRenderOpengl::get().getGlTextureId(m_tex);
    
    //renderapi

    return 0;
}

unsigned int RoxTexture::getUsedVmemSize()
{
    //renderapi

    return 0;
}

void RoxTexture::release()
{
    if(m_tex<0)
        return;

    getApiInterface().removeTexture(m_tex);
    IRoxRenderApi::State &s=getApiState();
    for(int i=0;i<s.max_layers;++i)
    {
        if(s.textures[i]==m_tex)
            s.textures[i]=-1;
    }

    *this=RoxTexture();
}

}
