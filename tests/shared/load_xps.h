//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/vector.h"

namespace nya_memory { class memory_reader; class tmp_buffer_ref; }
namespace nya_scene { class mesh; struct shared_mesh; struct shared_animation; typedef nya_memory::tmp_buffer_ref resource_data; }

struct xps_loader
{
public:
    struct vert
    {
        nya_math::vec3 pos;

        nya_math::vec4 color;

        nya_math::vec3 normal;
        nya_math::vec3 tangent;
        nya_math::vec3 bitangent;

        nya_math::vec4 tc01;
        nya_math::vec4 tc23;

        float bone_idx[4];
        float bone_weight[4];

        nya_math::vec2 &tc(int idx)
        {
            switch(idx)
            {
                case 1: return tc01.zw();
                case 2: return tc23.xy();
                case 3: return tc23.zw();
            }

            return tc01.xy();
        }
    };

    static void set_light_dir(const nya_math::vec3 &dir);

public:
    static bool load_mesh(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name);
    static bool load_mesh_ascii(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name);
    static bool load_pose(nya_scene::shared_animation &res,nya_scene::resource_data &data,const char* name);
};
