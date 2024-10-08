//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "scene.h"

namespace { nya_log::log_base *scene_log=0; }

namespace nya_scene
{

void set_log(nya_log::log_base *l) { scene_log=l; }

nya_log::log_base &log()
{
    if(!scene_log)
        return nya_log::log();

    return *scene_log;
}

}
