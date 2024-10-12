//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "scene/texture.h"

namespace nya_scene
{
    bool load_texture_libjpeg(shared_texture &res,resource_data &data,const char* name);
}

namespace nya_formats
{

struct jpeg
{
    enum color_mode
    {
        greyscale=1,
        rgb=3
    };

    int width;
    int height;
    color_mode channels;

    bool decode_header(const void *data,size_t size);
    bool decode_color(void *decoded_data) const; //decoded_data must be allocated with get_decoded_size()
    size_t get_decoded_size() const;

    jpeg(): width(0),height(0),data(0),data_size(0) {}

private:
    const void *data;
    size_t data_size;
};

bool encode_jpeg(const void *color_data,int width,int height,int channels,std::vector<char> &result,int quality=80);

}
