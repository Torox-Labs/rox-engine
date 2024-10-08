//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "statistics.h"

namespace nya_render
{

namespace { statistics stats; bool stats_enabled=false; }

void statistics::begin_frame() { stats=statistics(); stats_enabled=true; }
statistics &statistics::get() { return stats; }
bool statistics::enabled() { return stats_enabled; }

}
