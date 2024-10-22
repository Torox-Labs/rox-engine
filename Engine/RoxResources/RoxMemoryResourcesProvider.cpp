//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxMemoryResourcesProvider.h"
#include "RoxMemory/RoxAlignAlloc.h"
#include <cstring>

namespace RoxResources
{

bool RoxMemoryResourcesProvider::has(const char *name)
{
    if(!name)
        return false;

    RoxMemory::RoxLockGuardRead lock(m_mutex);

    for(int i=0;i<(int)m_entries.size();++i)
    {
        if(m_entries[i].name==name)
            return true;
    }

    return false;
}

int RoxMemoryResourcesProvider::getResourcesCount() { return (int)m_entries.size(); }

const char *RoxMemoryResourcesProvider::getResourceName(int idx)
{
    if(idx<0 || idx>=(int)m_entries.size())
        return 0;

    return m_entries[idx].name.c_str();
}

bool RoxMemoryResourcesProvider::add(const char *name,const void *data,size_t size)
{
    if(!name)
        return false;

    if(!data && size>0)
        return false;

    if(has(name))
        return false;

    void *new_data=RoxMemory::alignAlloc(size,16);
    memcpy(new_data,data,size);

    entry e;
    e.name=name;
    e.data=(char *)new_data;
    e.size=size;

    RoxMemory::RoxLockGuardWrite lock(m_mutex);

    m_entries.push_back(e);
    return true;
}

bool RoxMemoryResourcesProvider::remove(const char *name)
{
    if(!name)
        return false;

    RoxMemory::RoxLockGuardWrite lock(m_mutex);

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
    struct resource: public RoxResourceData
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

RoxResourceData *RoxMemoryResourcesProvider::access(const char *name)
{
    if(!name)
        return 0;

    RoxMemory::RoxLockGuardRead lock(m_mutex);

    for(int i=0;i<(int)m_entries.size();++i)
    {
        if(m_entries[i].name!=name)
            continue;

        return new resource(m_entries[i].data,m_entries[i].size);
    }

    return 0;
}

}
