// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project. The namespace has been renamed from nya_log to rox_log.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxLogger.h"
#include "RoxStdoutLog.h"

#include <stdarg.h>
#include <stdio.h>

namespace RoxLogger
{

namespace { RoxLoggerBase* current_log = new RoxStdoutLog(); }

RoxLoggerBase& noLogger()
{
    static RoxLoggerBase* l = new RoxLoggerBase();
    return* l;
}

void setLogger(RoxLoggerBase* l) { current_log = current_log ? l : &noLogger(); }
RoxLoggerBase& log() { return* current_log; }

RoxLoggerBase& log(const char* fmt, ...)
{
    va_list args,args_copy;
    va_start(args,fmt);
#ifdef _WIN32
    args_copy=args;
	va_copy(args_copy, args);
    const int len=_vscprintf(fmt,args)+1;
#endif
    std::string buf;
    buf.resize(len);
    printf(&buf[0],fmt,args_copy);
    *current_log<<buf;
    return *current_log;
}

}
