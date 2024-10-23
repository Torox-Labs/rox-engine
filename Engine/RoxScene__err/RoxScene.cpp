//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxScene.h"

namespace { RoxLogger::RoxLoggerBase *scene_log=0; }

namespace nya_scene
{

void set_log(RoxLogger::RoxLoggerBase *l) { scene_log=l; }

RoxLogger::RoxLoggerBase &log()
{
    if(!scene_log)
        return RoxLogger::log();

    return *scene_log;
}

}
