//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxLogger/RoxLogger.h"
#include "RoxLogger/RoxWarning.h"

namespace RoxScene
{

void setLog(RoxLogger::RoxLoggerBase *l);
RoxLogger::RoxLoggerBase &log();

}
