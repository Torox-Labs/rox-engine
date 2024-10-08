//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "scene/texture.h"
#include "memory/memory_reader.h"

//loads only bmp with alpha because almost nobody support alpha in bmp

namespace nya_scene
{
    static bool load_texture_bgra_bmp(shared_texture &res,resource_data &data,const char* name)
    {
        nya_memory::memory_reader reader(data.get_data(),data.get_size());
        if(!reader.test("BM",2))
            return false;

        typedef unsigned int uint;
        typedef unsigned short ushort;

        reader.seek(10);
        const uint data_offset=reader.read<uint>();
        const uint header_size=reader.read<uint>();
        if(header_size<40)
            return false;

        int width=reader.read<int>();
        int height=reader.read<int>();

        const ushort planes=reader.read<ushort>();
        if(planes!=1)
            return false;

        const ushort bpp=reader.read<ushort>();
        if(bpp!=32)
            return false;

        const uint compression=reader.read<uint>();
        if(compression!=0)
            return false;

        const uint image_size=reader.read<uint>();
        if(header_size==40 && image_size!=0) //what
            return false;

        reader.seek(data_offset);
        if(height<0)
        {
            height= -height;
            unsigned int *data=(unsigned int*)reader.get_data();
            const int top=width*(height-1);
            for(int offset=0;offset<width*height/2;offset+=width)
            {
                unsigned int *a=data+offset,*b=data+top-offset;
                for(int w=0;w<width;++w,++a,++b)
                    std::swap(*a,*b);
            }
        }
        else if(!height)
            return false;

        res.tex.build_texture(reader.get_data(),width,height,nya_render::texture::color_bgra);
        return true;
    }
}
