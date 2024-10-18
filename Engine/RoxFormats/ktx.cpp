//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "ktx.h"
#include "memory/memory_reader.h"
#include "resources/resources.h"
#include <stdint.h>

namespace nya_formats
{

struct ktx_header
{
    typedef uint32_t uint;
    uint endianess;
    uint gl_type;
    uint gl_type_size;
    uint gl_format;
    uint gl_internal_format;
    uint gl_base_internal_format;
    uint width;
    uint height;
    uint depth;
    uint array_elements_count;
    uint faces_count;
    uint mipmap_count;
    uint key_value_size;
};

size_t ktx::decode_header(const void *data,size_t size)
{
    *this=ktx();

    if(!data || size<128)
        return 0;

    typedef uint32_t uint;

    nya_memory::memory_reader reader(data,size);
    if(!reader.test("\xABKTX 11\xBB\r\n\x1A\n",12))
        return 0;

    const ktx_header header=reader.read<ktx_header>();
    if(header.endianess!=0x04030201)
        return 0;

    reader.skip(header.key_value_size);

    const bool is_cubemap=header.faces_count==6;
    if(is_cubemap || header.faces_count!=1)
        return 0;

    if(is_cubemap) //ToDo
        return 0;

    pixel_format pf;

    switch(header.gl_format)
    {
        case 0x1907: pf=rgb; break;
        case 0x1908: pf=rgba; break;
        case 0x80E1: pf=bgra; break;

        case 0:
        {
            switch(header.gl_internal_format)
            {
                case 0x8D64: pf=etc1; break;
                case 0x9274: pf=etc2; break;
                case 0x9278: pf=etc2_eac; break;
                case 0x9276: pf=etc2_a1; break;

                case 0x8c01: pf=pvr_rgb2b; break;
                case 0x8c00: pf=pvr_rgb4b; break;
                case 0x8c03: pf=pvr_rgba2b; break;
                case 0x8c02: pf=pvr_rgba4b; break;
                default:
                    return 0;
            }
        }
        break;

        default:
            return 0;
    }

    if(!header.mipmap_count)
        return 0;

    uint data_size=0;
    for(uint i=0,w=header.width,h=header.height;i<header.mipmap_count;++i,w>1?w/=2:w=1,h>1?h/=2:h=1)
    {
        if(pf<etc1)
            data_size+=w*h*(pf==rgb?3:4);
        else if(pf==pvr_rgb2b || pf==pvr_rgba2b)
            data_size+=((w>16?w:16)*(h>8?h:8)*2 + 7)/8;
        else if(pf==pvr_rgb4b || pf==pvr_rgba4b)
            data_size+=((w>8?w:8)*(h>8?h:8)*4 + 7)/8;
        else
            data_size += ((w+3)>>2) * ((h+3)>>2) * (pf==etc2_eac?16:8);
    }

    data_size+=header.mipmap_count*4;

    if(!reader.check_remained(data_size))
        return 0;

    width=header.width;
    height=header.height;
    this->data_size=data_size;
    this->data=reader.get_data();
    this->mipmap_count=header.mipmap_count;
    this->pf=pf;

    return reader.get_offset();
}

}
