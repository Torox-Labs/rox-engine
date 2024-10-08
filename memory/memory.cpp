//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "memory.h"

namespace { nya_log::log_base *memory_log=0; }

namespace nya_memory
{

void set_log(nya_log::log_base *l) { memory_log=l; }

nya_log::log_base &log()
{
    if(!memory_log)
        return nya_log::log();

    return *memory_log;
}

}
