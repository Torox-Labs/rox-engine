//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "memory_resources_provider.h"
#include "memory/align_alloc.h"
#include <string.h>

namespace nya_resources
{

bool memory_resources_provider::has(const char *name)
{
    if(!name)
        return false;

    nya_memory::lock_guard_read lock(m_mutex);

    for(int i=0;i<(int)m_entries.size();++i)
    {
        if(m_entries[i].name==name)
            return true;
    }

    return false;
}

int memory_resources_provider::get_resources_count() { return (int)m_entries.size(); }

const char *memory_resources_provider::get_resource_name(int idx)
{
    if(idx<0 || idx>=(int)m_entries.size())
        return 0;

    return m_entries[idx].name.c_str();
}

bool memory_resources_provider::add(const char *name,const void *data,size_t size)
{
    if(!name)
        return false;

    if(!data && size>0)
        return false;

    if(has(name))
        return false;

    void *new_data=nya_memory::align_alloc(size,16);
    memcpy(new_data,data,size);

    entry e;
    e.name=name;
    e.data=(char *)new_data;
    e.size=size;

    nya_memory::lock_guard_write lock(m_mutex);

    m_entries.push_back(e);
    return true;
}

bool memory_resources_provider::remove(const char *name)
{
    if(!name)
        return false;

    nya_memory::lock_guard_write lock(m_mutex);

    for(int i=0;i<(int)m_entries.size();++i)
    {
        if(m_entries[i].name!=name)
            continue;

        m_entries.erase(m_entries.begin()+i);
        return true;
    }

    return false;
}

namespace
{
    struct resource: public resource_data
    {
        const char *data;
        size_t size;

        resource(const char *data,size_t size): data(data),size(size) {}

        size_t get_size() { return size; }

        virtual bool read_all(void *out_data)
        {
            if(!out_data)
                return false;

            memcpy(out_data,data,size);
            return true;
        }

        virtual bool read_chunk(void *out_data,size_t out_size,size_t offset)
        {
            if(!out_data)
                return false;

            if(out_size+offset>size || out_size>size)
                return false;

            memcpy(out_data,data+offset,size);
            return true;
        }

        void release() { delete this; }
    };
}

resource_data *memory_resources_provider::access(const char *name)
{
    if(!name)
        return 0;

    nya_memory::lock_guard_read lock(m_mutex);

    for(int i=0;i<(int)m_entries.size();++i)
    {
        if(m_entries[i].name!=name)
            continue;

        return new resource(m_entries[i].data,m_entries[i].size);
    }

    return 0;
}

}
