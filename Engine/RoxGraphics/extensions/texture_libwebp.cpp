//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//requires: libwebp https://github.com/webmproject/libwebp

#include "texture_libwebp.h"
#include "scene/scene.h"
#include "memory/memory_reader.h"
#include "webp/decode.h"
#include "render/bitmap.h"

namespace nya_scene
{

bool load_texture_libwebp(shared_texture &res,resource_data &data,const char* name)
{
    nya_formats::webp webp;
    if(!webp.decode_header(data.get_data(),data.get_size()))
        return false;

    if(!webp.width || !webp.height)
    {
        log()<<"unable to load webp texture: zero width or height in file "<<name<<"\n";
        return false;
    }

    nya_render::texture::color_format cf;
    switch(webp.channels)
    {
        case nya_formats::webp::rgb: cf=nya_render::texture::color_rgb; break;
        case nya_formats::webp::rgba: cf=nya_render::texture::color_rgba; break;

        default:
            log()<<"unable to load texture with libwebp: unsupported "<<webp.channels<<" components in file "<<name<<"\n";
            return false;
    }

    nya_memory::tmp_buffer_scoped buf(webp.get_decoded_size());
    if(!webp.decode_color(buf.get_data()))
        return false;

    res.tex.build_texture(buf.get_data(),webp.width,webp.height,cf);
    texture::read_meta(res,data);

    return true;
}

}

namespace { struct riff_chunk { char four_cc[4]; uint32_t size; }; }

namespace nya_formats
{

bool webp::decode_header(const void *data,size_t size)
{
    *this=webp();

    if(!data)
        return false;

    if(WebPGetInfo((const uint8_t*)data,size,&width,&height)!=1)
        return false;

    channels=rgb;
    nya_memory::memory_reader r(data,size);
    r.skip(12);//header
    while(r.get_remained()>0)
    {
        riff_chunk rc=r.read<riff_chunk>();
        if(memcmp(rc.four_cc,"ALPH",4)==0)
        {
            channels=rgba;
            break;
        }
        r.skip(rc.size);
    }

    this->data=data;
    data_size=size;
    return true;
}

bool webp::decode_color(void *decoded_data) const
{
    if(channels==rgb)
        WebPDecodeRGBInto((const uint8_t*)data,data_size,(uint8_t*)decoded_data,get_decoded_size(),width*3);
    else if(channels==rgba)
        WebPDecodeRGBAInto((const uint8_t*)data,data_size,(uint8_t*)decoded_data,get_decoded_size(),width*4);
    else
        return false;

    nya_render::bitmap_flip_vertical((unsigned char *)decoded_data,width,height,channels);
    return true;
}

size_t webp::get_decoded_size() const { return width*height*channels; }

}
