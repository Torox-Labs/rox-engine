//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "scene/texture.h"

namespace nya_scene
{
    bool load_texture_libpng(shared_texture &res,resource_data &data,const char* name);
}

namespace nya_formats
{
    bool encode_png(const void *color_data,int width,int height,int channels,std::vector<char> &result);
}
