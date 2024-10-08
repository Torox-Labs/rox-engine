//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "resources.h"
#include "file_resources_provider.h"
//#include "system/system.h"
#include <string.h>

#if defined(_WIN32)
    #define strcasecmp _stricmp
#endif

namespace nya_resources
{

namespace
{
    resources_provider *res_provider=0;
    nya_log::log_base *resources_log=0;

    file_resources_provider &default_provider()
    {
        static file_resources_provider *fprov=new file_resources_provider();
        return *fprov;
    }
}

void set_resources_path(const char *path)
{
    res_provider=&default_provider();
    default_provider().set_folder(path);
}

void set_resources_provider(resources_provider *provider)
{
    res_provider=provider;
}

resources_provider &get_resources_provider()
{
    if(!res_provider)
    {
        res_provider=&default_provider();
        //default_provider().set_folder(nya_system::get_app_path());
    }

    return *res_provider;
}

nya_memory::tmp_buffer_ref read_data(const char *name)
{
    resource_data *r=get_resources_provider().access(name);
    if(!r)
        return nya_memory::tmp_buffer_ref();

    nya_memory::tmp_buffer_ref result;
    result.allocate(r->get_size());
    if(!r->read_all(result.get_data()))
        result.free();
    r->release();
    return result;
}

void set_log(nya_log::log_base *l) { resources_log=l; }

nya_log::log_base &log()
{
    if(!resources_log)
        return nya_log::log();

    return *resources_log;
}

bool check_extension(const char *name,const char *ext)
{
    if(!name || !ext)
        return false;

    const size_t name_len=strlen(name),ext_len=strlen(ext);
    if(ext_len>name_len)
        return false;

    return strcasecmp(name+name_len-ext_len,ext)==0;
}

}
