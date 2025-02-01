// Updated By the ROX_ENGINE
// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Drop support for METRO, FLUENT style.
// Drop Support for Android, iOS
// Update the code to be compatible with the latest version of the engine.
// Optimasation and code cleaning for a better performance.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxSystem.h"

#include <iomanip>

#if defined __APPLE__
#elif defined _WIN32
    #include <windows.h>
    #include <cstring>
	#include <ctime>

    #if defined(_MSC_VER) && _MSC_VER >= 1900
		#if _WIN32_WINNT >= _WIN32_WINNT_WIN10

			#include "winapifamily.h"

		#endif
    #endif
#endif


#ifndef _WIN32
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>

#endif

namespace
{
    RoxLogger::RoxLoggerBase *system_log=0;
}

namespace RoxSystem
{
void setLog(RoxLogger::RoxLoggerBase *l) {
    system_log=l;
}

RoxLogger::RoxLoggerBase &log()
{
    if(!system_log)
        return RoxLogger::log();
    
    return *system_log;
}

const char *getAppPath()
{
    const size_t max_path=4096;
    static char path[max_path]="";
    static bool has_path=false;
    if(!has_path)
    {
#ifdef __APPLE__
#elif defined _WIN32

        GetModuleFileNameA(0,
                           path,
                           max_path);
        
        char *last_slash=strrchr(path,
                                 '\\');
        if(last_slash)
            *(last_slash+1) = 0;

        for(size_t i=0;i<max_path;++i)
        {
            if(path[i]=='\\')
                path[i]='/';
        }

#elif EMSCRIPTEN
#else
#endif
        has_path=true;
    }
    
    return path;
}

const char *getUserPath()
{
    const size_t max_path=4096;
    static char path[max_path]="";
    static bool has_path=false;
    if(!has_path)
    {
#ifdef _WIN32

        const char *p=getenv("APPDATA");
        if(!p)
            return 0;
        
        strcpy(path,
               p);
        strcat(path,
               "/");

        for(size_t i=0;i<max_path;++i)
        {
            if(path[i]=='\\')
                path[i]='/';
        }

#elif TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
#endif
        has_path=true;
    }
    
    return path;
}

#ifdef _WIN32

//#pragma comment ( lib, "WINMM.LIB" )
#pragma comment( lib, "winmm.lib")   

unsigned long getTime()
{

    return timeGetTime();
}

#else
#endif

}

