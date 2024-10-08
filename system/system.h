//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "log/log.h"

namespace nya_system
{

void set_log(nya_log::log_base *l);
nya_log::log_base &log();

const char *get_app_path();
const char *get_user_path();
unsigned long get_time();

void emscripten_sync_fs();
bool emscripten_sync_fs_finished();

}
