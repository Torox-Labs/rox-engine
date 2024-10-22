//nya-e(C) nyan.developer@gmail.com released under the MIT l(see LICENSE)

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

    RoxResourcesProvider& gresourcesProvider()
    {
        if(!res_provider)
        {
            res_provider = &default_provider();
            //default_provider().sfolder(nya_system::gappPath());
        }

        return *res_provider;
    }

    RoxMemory::RoxTmpBufferRef rdata(const char* name)
    {
        RoxResourceData* r = gresourcesProvider().access(name);
        if(!r)
            return RoxMemory::RoxTmpBufferRef();

        RoxMemory::RoxTmpBufferRef result;
        result.allocate(r->getSize());
        if(!r->readAll(result.getData()))
            result.free();
        r->release();
        return result;
    }

    void slog(RoxLogger::RoxLoggerBase* l) { resources_log = l; }

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
