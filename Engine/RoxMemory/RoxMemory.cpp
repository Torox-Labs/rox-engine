//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxMemory.h"

namespace { RoxLogger::RoxLoggerBase *memory_log=0; }

namespace RoxMemory
{

void set_log(RoxLogger::RoxLoggerBase*l) { memory_log=l; }

RoxLogger::RoxLoggerBase&log()
{
    if(!memory_log)
        return RoxLogger::log();

    return *memory_log;
}

}
