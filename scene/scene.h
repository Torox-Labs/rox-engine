//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "log/log.h"
#include "log/warning.h"

namespace nya_scene
{

void set_log(nya_log::log_base *l);
nya_log::log_base &log();

}
