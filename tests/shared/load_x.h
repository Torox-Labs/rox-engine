//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/vector.h"
#include <vector>
#include <string>

namespace nya_memory { class tmp_buffer_ref; }
namespace nya_scene { class mesh; struct shared_mesh; typedef nya_memory::tmp_buffer_ref resource_data; }

struct x_loader
{
public:
    struct vert
    {
        nya_math::vec3 pos;
        nya_math::vec3 normal;
        nya_math::vec2 tc;
    };

public:
    static bool load_text(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name);
};
