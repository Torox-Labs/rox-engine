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

#include "output_stream.h"
#include <stdio.h>

namespace rox_log
{

namespace { char buf[512]; }

ostream_base &ostream_base::operator << (long int a) { sprintf(buf,"%ld", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (unsigned long int a) { sprintf(buf,"%lu", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (float a) { sprintf(buf,"%f", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (double a) { sprintf(buf,"%f", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (const char *a) { output(a?a:"NULL"); return *this; }

ostream_base &ostream_base::operator << (int a) { sprintf(buf,"%d", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (unsigned int a) { sprintf(buf,"%u", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (short int a) { sprintf(buf,"%d", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (unsigned short int a) { sprintf(buf,"%u", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (long long int a) { sprintf(buf,"%lld", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (unsigned long long int a) { sprintf(buf,"%llu", a); output(buf); return *this; }
ostream_base &ostream_base::operator << (const std::string &a) { output(a.c_str()); return *this; }

}
