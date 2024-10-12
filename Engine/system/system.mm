//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#ifdef __APPLE__
  #include "TargetConditionals.h"
  #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
    #import <Foundation/Foundation.h>

    bool get_ios_user_path(char *path)
    {
        NSArray *paths=NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES);
        if(!paths)
            return false;

        NSString *documentsDirectory=[paths objectAtIndex:0];
        if(!documentsDirectory)
            return false;

        strcpy(path,documentsDirectory.UTF8String);
        strcat(path,"/");
        return true;
    }
  #endif
#endif
