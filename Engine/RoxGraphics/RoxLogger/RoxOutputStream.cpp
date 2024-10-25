// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
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

#include "RoxOutputStream.h"
#include <stdio.h>

namespace RoxLogger
{

namespace { char buf[512]; }

RoxOstreamBase &RoxOstreamBase::operator << (long int a) { printf(buf,"%ld", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (unsigned long int a) { printf(buf,"%lu", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (float a) { printf(buf,"%f", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (double a) { printf(buf,"%f", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (const char *a) { output(a?a:"NULL"); return *this; }

RoxOstreamBase &RoxOstreamBase::operator << (int a) { printf(buf,"%d", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (unsigned int a) { printf(buf,"%u", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (short int a) { printf(buf,"%d", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (unsigned short int a) { printf(buf,"%u", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (long long int a) { printf(buf,"%lld", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (unsigned long long int a) { printf(buf,"%llu", a); output(buf); return *this; }
RoxOstreamBase &RoxOstreamBase::operator << (const std::string &a) { output(a.c_str()); return *this; }

}
