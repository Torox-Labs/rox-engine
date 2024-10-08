//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "scene/texture.h"

namespace nya_scene
{
    bool load_texture_il(shared_texture &res,resource_data &data,const char* name);
    bool save_texture_il(const texture &tex, const char *name);
}
