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

#include "android_log.h"

#ifdef __ANDROID__
    #include <android/log.h>
    #include "memory/mutex.h"
#endif

namespace rox_log
{

void android_log::output(const char *str)
{
#ifdef __ANDROID__
    if(!str)
        return;

    static nya_memory::mutex mutex;
    nya_memory::lock_guard guard(mutex);

    m_buf.append(str);
    for(size_t i=m_buf.find("\n");i!=std::string::npos;i=m_buf.find("\n"))
    {
        std::string tmp=m_buf.substr(0,i);
        m_buf=m_buf.substr(i+1);
        __android_log_print(ANDROID_LOG_INFO,m_id.c_str(),"%s",tmp.c_str());
    }
#endif
}

}
