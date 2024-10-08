//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/vector.h"
#include <vector>
#include <string>

namespace nya_memory { class tmp_buffer_ref; }
namespace nya_scene { class mesh; class shared_mesh; typedef nya_memory::tmp_buffer_ref resource_data; }

struct tdcg_loader
{
public:
    struct vert
    {
        nya_math::vec3 pos;
        nya_math::vec3 normal;
        nya_math::vec2 tc;

        float bone_idx[4];
        float bone_weight[4];
    };

public:
    static bool load_hardsave(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name);
};
