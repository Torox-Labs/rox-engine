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

#pragma once

#include <string>

namespace RoxLogger
{

class RoxOstreamBase
{
protected:
    virtual void output(const char *str) {}

public:
    RoxOstreamBase &operator << (long int);
    RoxOstreamBase &operator << (unsigned long int);
    RoxOstreamBase &operator << (float);
    RoxOstreamBase &operator << (double);
    RoxOstreamBase &operator << (const char *);

    RoxOstreamBase &operator << (int a);
    RoxOstreamBase &operator << (unsigned int a);
    RoxOstreamBase &operator << (short int a);
    RoxOstreamBase &operator << (unsigned short int a);
    RoxOstreamBase &operator << (long long int a);
    RoxOstreamBase &operator << (unsigned long long int a);
    RoxOstreamBase &operator << (const std::string &a);
};

}
