//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <stddef.h>

namespace nya_formats
{

struct ktx
{
    unsigned int width;
    unsigned int height;

    unsigned int mipmap_count;

    enum pixel_format
    {
        rgb,
        rgba,
        bgra,

        etc1,
        etc2,
        etc2_eac,
        etc2_a1,

        pvr_rgb2b,
        pvr_rgb4b,
        pvr_rgba2b,
        pvr_rgba4b,
    };

    pixel_format pf;

    const void *data;
    size_t data_size;

    ktx(): width(0),height(0),mipmap_count(0),data(0),data_size(0) {}

public:
    size_t decode_header(const void *data,size_t size); //0 if invalid
};

}
