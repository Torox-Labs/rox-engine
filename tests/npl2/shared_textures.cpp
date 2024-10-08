//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "shared_textures.h"
#include "resources/resources.h"
#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"

namespace nya_resources
{

void rgb_to_bgr(unsigned char *data,size_t data_size)
{
    unsigned char *data2=data+2;
    for(size_t i=0;i<data_size;i+=3)
    {
        unsigned char tmp=data[i];
        data[i]=data2[i];
        data2[i]=tmp;
    }
}

void flip_vertical(unsigned char *data,int width,int height,int bpp)
{
    const int line_size=width*bpp;
    const int top=line_size*(height-1);
    const int half=line_size*height/2;
    for(int offset=0;offset<half;offset+=line_size)
    {
        unsigned char *ha=data+offset;
        unsigned char *hb=data+top-offset;

        for(int w=0;w<line_size;w+=bpp)
        {
            unsigned char *a=ha+w;
            unsigned char *b=hb+w;
            unsigned char tmp[4];
            memcpy(tmp,a,bpp);
            memcpy(a,b,bpp);
            memcpy(b,tmp,bpp);
        }
    }
}

bool shared_textures_manager::fill_resource(const char *name,nya_render::texture &res)
{
    if(!name)
    {
        log()<<"unable to load texture: invalid name\n";
        return false;
    }

    nya_resources::resource_data *file_data = nya_resources::get_resources_provider().access(name);
    if(!file_data)
    {
        log()<<"unable to load texture: unable to acess resource\n";
        return false;
    }

    const size_t data_size=file_data->get_size();
    nya_memory::tmp_buffer_scoped texture_data(data_size);
    file_data->read_all(texture_data.get_data());
    file_data->release();

    nya_memory::memory_reader reader(texture_data.get_data(),data_size);

    const char id_length=reader.read<char>();
    reader.skip(id_length);

    const char colourmaptype=reader.read<char>();
    if(colourmaptype!=0)
        return false;

    const char datatypecode=reader.read<char>();
    const short colourmaporigin=reader.read<short>();
    if(colourmaporigin!=0)
        return false;

    const short colourmaplength=reader.read<short>();
    if(colourmaplength!=0)
        return false;

    const char colourmapdepth=reader.read<char>();
    if(colourmapdepth!=0)
        return false;

    //const short x_origin=
    reader.read<short>();
    //const short y_origin=
    reader.read<short>();
    const short width=reader.read<short>();
    const short height=reader.read<short>();
    const char bitsperpixel=reader.read<char>();
    //const char imagedescriptor=
    reader.read<char>();

    int channels = 0;
    bool rle = false;

    if(bitsperpixel==32)
    {
        if(datatypecode==10)
        {
            channels=4;
            rle=true;
        }
        else if(datatypecode==2)
            channels = 4;
    }
    else if(bitsperpixel==24)
    {
        if(datatypecode==10)
        {
            channels=3;
            rle=true;
        }
        else if(datatypecode==2)
            channels = 3;
    }
    else if(bitsperpixel==8)
    {
        if(datatypecode==11)
        {
            channels=1;
            rle=true;
        }
        else if(datatypecode==3)
            channels=1;
    }

    if(channels==0)
        return false;

    const size_t color_data_size=width*height*channels;

    nya_render::texture::color_format color_format;
    if(channels==4)
        color_format=nya_render::texture::color_bgra;
    else if(channels==3)
        color_format=nya_render::texture::color_rgb;
    else if(channels==1)
        color_format=nya_render::texture::greyscale;
    else
        return false;

    if(rle)
    {
        nya_memory::tmp_buffer_scoped color_data(color_data_size);
        
        const unsigned char *cur=(unsigned char *)reader.get_data();
        
        for(size_t i=0;i<color_data_size;) 
        {
            if(*cur & 0x80)
            {
                unsigned char len= *cur -127; 
                ++cur;
                
                for(unsigned char j=0;j<len;++j,i+=channels)
                    memcpy(color_data.get_data(i),cur,channels);
                
                cur+=channels;
            }
            else // raw
            {
                unsigned char len= *cur +1; 
                ++cur;
                
                for(unsigned char j=0;j<len;++j,i+=channels,cur+=channels)
                    memcpy(color_data.get_data(i),cur,channels);
            }
        }

        if(color_format==nya_render::texture::color_rgb)
            rgb_to_bgr((unsigned char*)color_data.get_data(),color_data_size);

        flip_vertical((unsigned char*)color_data.get_data(),width,height,channels);

        res.build_texture(color_data.get_data(),width,height,color_format);
    }
    else
    {
        if(!reader.check_remained(color_data_size))
            return false;

        if(color_format==nya_render::texture::color_rgb)
            rgb_to_bgr((unsigned char*)reader.get_data(),color_data_size);

        flip_vertical((unsigned char*)reader.get_data(),width,height,channels);

        res.build_texture(reader.get_data(),width,height,color_format);
    }

    return true;
}

bool shared_textures_manager::release_resource(nya_render::texture &res)
{
    res.release();
    return true;
}

shared_textures_manager &get_shared_textures()
{
    static shared_textures_manager manager;
    return manager;
}

}
