// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "RoxResources.h"
#include "RoxFileResourcesProvider.h"
#include "RoxSystem/RoxSystem.h"
#include <cstring>

#if defined(_WIN32)
#define strcasecmp _stricmp
#endif

namespace RoxResources
{

    namespace
    {
        RoxResourcesProvider* res_provider= 0;
        RoxLogger::RoxLoggerBase* resources_log= 0;

        RoxFileResourcesProvider& default_provider()
        {
            static RoxFileResourcesProvider* file_provider = new RoxFileResourcesProvider();
            return *file_provider;
        }
    }

    void setResourcesPath(const char* path)
    {
        res_provider = &default_provider();
        default_provider().setFolder(path);
    }

    void setResourcesProvider(RoxResourcesProvider* provider)
    {
        res_provider = provider;
    }

    RoxResourcesProvider& getResourcesProvider()
    {
        if(!res_provider)
        {
            res_provider = &default_provider();
            //default_provider().sfolder(nya_system::gappPath());
        }

        return *res_provider;
    }

    RoxMemory::RoxTmpBufferRef readData(const char* name)
    {
        RoxResourceData* r = getResourcesProvider().access(name);
        if(!r)
            return RoxMemory::RoxTmpBufferRef();

        RoxMemory::RoxTmpBufferRef result;
        result.allocate(r->getSize());
        if(!r->readAll(result.getData()))
            result.free();
        r->release();
        return result;
    }

    void setLog(RoxLogger::RoxLoggerBase* l) { resources_log = l; }

    RoxLogger::RoxLoggerBase& l()
    {
        if(!resources_log)
            return RoxLogger::log();

        return *resources_log;
    }

    bool checkExtension(const char* name, const char* ext)
    {
        if(!name || !ext)
            return false;

        const size_t name_len = strlen(name), ext_len = strlen(ext);
        if(ext_len > name_len)
            return false;

        return strcasecmp(name + name_len - ext_len, ext) == 0;
    }

}
