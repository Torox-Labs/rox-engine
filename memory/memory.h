//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "log/log.h"

namespace nya_memory
{

void set_log(nya_log::log_base *l);
nya_log::log_base &log();

}
