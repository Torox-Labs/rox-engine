//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "system.h"

#ifdef __APPLE__
    #include <mach-o/dyld.h>
    #include "TargetConditionals.h"
    #include <string>

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        bool get_ios_user_path(char *path);
    #endif
#elif defined _WIN32
    #include <windows.h>
    #include <string.h>

    #if defined(_MSC_VER) && _MSC_VER >= 1700
      #if _WIN32_WINNT >= _WIN32_WINNT_WIN8 && !_USING_V110_SDK71_
          #include "winapifamily.h"
          #if defined(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
          #elif defined(WINAPI_PARTITION_PHONE) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE)
              #define WINDOWS_PHONE8
              #define WINDOWS_METRO
          #elif defined(WINAPI_PARTITION_APP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
              #define WINDOWS_METRO
          #endif
      #endif
    #endif
#endif

#ifndef _WIN32
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
    #include <string.h>
#endif

#ifdef EMSCRIPTEN
    #include <emscripten/emscripten.h>
#endif

namespace
{
    nya_log::log_base *system_log=0;

#ifdef __ANDROID__
    std::string android_user_path;
#endif
}

namespace nya_system
{

#ifdef __ANDROID__
void set_android_user_path(const char *path) { android_user_path.assign(path?path:""); }
#endif

void set_log(nya_log::log_base *l) { system_log=l; }

nya_log::log_base &log()
{
    if(!system_log)
        return nya_log::log();

    return *system_log;
}

const char *get_app_path()
{
    const size_t max_path=4096;
    static char path[max_path]="";
    static bool has_path=false;
    if(!has_path)
    {
#ifdef __APPLE__
        uint32_t path_length=max_path;
        _NSGetExecutablePath(path,&path_length);

        std::string path_str(path);
        size_t p=path_str.rfind(".app");

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        size_t p2=path_str.find("/",p);
    #else
        size_t p2=path_str.rfind("/",p);
    #endif

        if(p2!=std::string::npos)
            path[p2+1]='\0';
        else
            path[0]='\0';
#elif defined _WIN32
	#ifdef WINDOWS_METRO
        auto current=Windows::ApplicationModel::Package::Current;
        if(!current)
            return 0;

		auto local=current->InstalledLocation;
        if(!local)
            return 0;

        Platform::String^ ilp=local->Path;
        const wchar_t *wp=ilp->Data();
        const char *p=path;
        for(size_t i=0;i<local->Path->Length();++i) //ToDo
            path[i]=(char)wp[i];

        path[local->Path->Length()]='/';
        path[local->Path->Length()+1]=0;
	#else
        GetModuleFileNameA(0,path,max_path);

        char *last_slash=strrchr(path,'\\');
        if(last_slash)
            *(last_slash+1) = 0;
	#endif
        for(size_t i=0;i<max_path;++i)
        {
            if(path[i]=='\\')
                path[i]='/';
        }
#elif EMSCRIPTEN
        strcpy(path,"/");
#else
        readlink("/proc/self/exe",path,max_path);

        char *last_slash=strrchr(path,'/');
        if(last_slash)
            *(last_slash+1)=0;
#endif
        has_path=true;
    }

    return path;
}

const char *get_user_path()
{
#ifdef __ANDROID__
    return android_user_path.c_str();
#endif

    const size_t max_path=4096;
    static char path[max_path]="";
    static bool has_path=false;
    if(!has_path)
    {
#ifdef _WIN32
    #ifdef WINDOWS_METRO
        auto current=Windows::Storage::ApplicationData::Current;
        if(!current)
            return 0;

        auto local=current->LocalFolder;
        if(!local)
            return 0;

        Platform::String^ ilp=local->Path;
        const wchar_t *wp=ilp->Data();
        const char *p=path;
        for(size_t i=0;i<local->Path->Length();++i) //ToDo
            path[i]=(char)wp[i];

        path[local->Path->Length()]='/';
        path[local->Path->Length()+1]=0;
    #else
        const char *p=getenv("APPDATA");
        if(!p)
            return 0;

        strcpy(path,p);
        strcat(path,"/");
    #endif
        for(size_t i=0;i<max_path;++i)
        {
            if(path[i]=='\\')
                path[i]='/';
        }
#elif EMSCRIPTEN
        strcpy(path,"/.nya/");
#elif TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        if(!get_ios_user_path(path))
            return 0;
#else
        const char *p=getenv("HOME");
        if (!p)
            p=getpwuid(getuid())->pw_dir;
        if(!p)
            return 0;

        strcpy(path,p);
        strcat(path,"/");
#endif
        has_path=true;
    }

    return path;
}

#ifdef _WIN32
  #ifdef WINDOWS_METRO
    unsigned long get_time()
    {
        static LARGE_INTEGER freq;
        static bool initialised=false;
        if(!initialised)
        {
            QueryPerformanceFrequency(&freq);
            initialised=true;
        }

        LARGE_INTEGER time;
        QueryPerformanceCounter(&time);

        return unsigned long(time.QuadPart*1000/freq.QuadPart);
    }
  #else
    #include "time.h"

    #pragma comment ( lib, "WINMM.LIB"  )

    unsigned long get_time()
    {
        return timeGetTime();
    }
  #endif
#else

#include <sys/time.h>

unsigned long get_time()
{
    timeval tim;
    gettimeofday(&tim, 0);
    unsigned long sec=(unsigned long)tim.tv_sec;
    return (sec*1000+(tim.tv_usec/1000));
}

#endif

namespace { bool is_fs_ready=true; }

#ifdef EMSCRIPTEN
extern "C" __attribute__((used)) void emscripten_on_sync_fs_finished() { is_fs_ready=true;  }
#endif

void emscripten_sync_fs()
{
#ifdef EMSCRIPTEN
    is_fs_ready=false;
    EM_ASM(FS.syncfs(false,function(err){ ccall('emscripten_on_sync_fs_finished','v'); }););
#endif
}

bool emscripten_sync_fs_finished() { return is_fs_ready; }

}

