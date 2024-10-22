//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxLogger/RoxLogger.h"

namespace RoxMemory
{

void set_log(RoxLogger::RoxLoggerBase *l);
RoxLogger::RoxLoggerBase&log();

}
