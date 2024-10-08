//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace nya_memory { class tmp_buffer_ref; }
namespace nya_scene { struct shared_animation; typedef nya_memory::tmp_buffer_ref resource_data; }

class vmd_loader
{
public:
    static bool load(nya_scene::shared_animation &res,nya_scene::resource_data &data,const char* name);
    static bool load_pose(nya_scene::shared_animation &res,nya_scene::resource_data &data,const char* name);

    const static char *ik_prefix;
    const static char *display_curve;
};
