//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "scene/texture.h"

namespace nya_scene
{
    bool load_texture_libwebp(shared_texture &res,resource_data &data,const char* name);
}

namespace nya_formats
{

struct webp
{
    enum color_mode
    {
        rgb=3,
        rgba=4
    };

    int width;
    int height;
    color_mode channels;

    bool decode_header(const void *data,size_t size);
    bool decode_color(void *decoded_data) const; //decoded_data must be allocated with get_decoded_size()
    size_t get_decoded_size() const;

    webp(): width(0),height(0),data(0),data_size(0) {}

private:
    const void *data;
    size_t data_size;
};

}

