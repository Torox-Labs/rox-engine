//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

int nya_main(int argc,const char *argv[]);

#ifdef _WIN32
  #include <windows.h>

  #if defined(_MSC_VER) && _MSC_VER >= 1700
    #if _WIN32_WINNT >= _WIN32_WINNT_WIN8 && !_USING_V110_SDK71_
        #include "winapifamily.h"
        #if defined(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        #elif defined(WINAPI_PARTITION_PHONE) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE)
            #define WINDOWS_METRO
        #elif defined(WINAPI_PARTITION_APP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
            #define WINDOWS_METRO
        #endif
    #endif
  #endif
#endif

#ifdef WINDOWS_METRO

int main( Platform::Array<Platform::String^>^ args)
{
    //ToDo: args

    return nya_main(0,0);
}

#else
  #ifdef _MSC_VER
    #pragma comment(linker, "/ENTRY:mainCRTStartup")
  #endif

int main(int argc,const char *argv[])
{
    return nya_main(argc,argv);
}

#endif
