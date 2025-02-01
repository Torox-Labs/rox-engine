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

#include "RoxPlainFileLog.h"
#include <stdio.h>

namespace RoxLogger
{

void RoxPlainFileLog::output(const char *str)
{
    if(FILE *f=fopen(m_file_name.c_str(),"a+"))
    {
        fprintf(f,"%s",str?str:"NULL");
        fclose(f);
    }
}

bool RoxPlainFileLog::open(const char *file_name,bool clear)
{
    if(FILE *f=fopen(file_name,clear?"w+":"a+"))
    {
        fclose(f);
        m_file_name.assign(file_name);
        return true;
    }

    m_file_name.clear();
    return false;
}

void RoxPlainFileLog::close() { m_file_name.clear(); }

}
