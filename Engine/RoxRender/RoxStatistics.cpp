//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxStatistics.h"

namespace RoxRender
{

namespace { Statistics stats; bool stats_enabled=false; }

void Statistics::beginFrame() { stats=Statistics(); stats_enabled=true; }
Statistics &Statistics::get() { return stats; }
bool Statistics::enabled() { return stats_enabled; }

}
