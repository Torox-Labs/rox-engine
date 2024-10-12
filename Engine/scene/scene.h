//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "logger/log.h"
#include "logger/warning.h"

namespace nya_scene
{

void set_log(rox_log::log_base *l);
rox_log::log_base &log();

}