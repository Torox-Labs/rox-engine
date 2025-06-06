// Updated By the ROX_ENGINE
// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// The namespace has been renamed from nya_system to rox_system.
// adding get_title function that return the title of the window
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.
 
#pragma once

#include "RoxLogger/RoxLogger.h"

namespace RoxSystem
{

void setLogger(RoxLogger::RoxLoggerBase *l);
RoxLogger::RoxLoggerBase &log();

const char *getAppPath();
const char *getUserPath();
unsigned long getTime();
//const char* getPlatformName();

}
