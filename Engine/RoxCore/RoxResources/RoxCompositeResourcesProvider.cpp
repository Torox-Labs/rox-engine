// Updated By the ROX_ENGINE
// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
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

#include "RoxCompositeResourcesProvider.h"
#include "RoxMemory/RoxPool.h"

#include <set>
#include <algorithm>
#include <cstring>

namespace RoxResources
{

inline std::string fix_name(const std::string &name)
{
    std::string name_str(name);
    for(size_t i=0;i<name_str.size();++i)
    {
        if(name_str[i]=='\\')
            name_str[i]='/';
    }

    std::string out;
    char prev=0;
    for(size_t i=0;i<name_str.size();++i)
    {
        if(prev=='/' && name_str[i]=='/')
            continue;

        prev=name_str[i];
        out.push_back(prev);
    }
    
    return out;
}

void RoxCompositeResourcesProvider::addProvider(RoxResourcesProvider *provider,const char *folder)
{
    if(!provider)
    {
        RoxLogger::log()<<"unable to add provider: invalid provider\n";
        return;
    }

    RoxMemory::RoxLockGuardWrite lock(m_mutex);

    for(size_t i=0;i<m_providers.size();++i)
    {
        if(m_providers[i].first!=provider)
            continue;

        std::swap(m_providers[i],m_providers.back());
        rebuildCache();
        return;
    }

    m_update_names=true;
    m_providers.push_back(std::make_pair(provider,(folder && folder[0])?fix_name(std::string(folder)+"/"):""));
    if(m_cache_entries)
        cacheProvider((int)m_providers.size()-1);
}

void RoxCompositeResourcesProvider::removeProviders()
{
    RoxMemory::RoxLockGuardWrite lock(m_mutex);

    m_providers.clear();
    m_resource_names.clear();
    m_cached_entries.clear();
}

RoxResourceData *RoxCompositeResourcesProvider::access(const char *resource_name)
{
    if(!resource_name)
    {
        RoxLogger::log()<<"unable to access composite entry: invalid name\n";
        return 0;
    }

    RoxMemory::RoxLockGuardRead lock(m_mutex);

    if(!m_cache_entries)
    {
        for(size_t i=0;i<m_providers.size();++i)
        {
            const char *name=resource_name;
            if(!m_providers[i].second.empty())
            {
                if(strncmp(resource_name,m_providers[i].second.c_str(),m_providers[i].second.size())!=0)
                    continue;

                name+=m_providers[i].second.size();
            }

            if(m_providers[i].first->has(name))
                return m_providers[i].first->access(name);
        }
        return 0;
    }

    entries_map::iterator it;

    if(m_ignore_case)
    {
        std::string res_str=fix_name(resource_name);
        std::transform(res_str.begin(),res_str.end(),res_str.begin(),::tolower);

        it=m_cached_entries.find(res_str);
    }
    else
        it=m_cached_entries.find(fix_name(resource_name));

    if(it==m_cached_entries.end())
    {
        RoxLogger::log()<<"unable to access composite entry "<<resource_name
                <<": not found\n";
        return 0;
    }

    return m_providers[it->second.prov_idx].first->access(it->second.original_name.c_str());
}

bool RoxCompositeResourcesProvider::has(const char *resource_name)
{
    if(!resource_name)
        return false;

    RoxMemory::RoxLockGuardRead lock(m_mutex);

    if(!m_cache_entries)
    {
        for(size_t i=0;i<m_providers.size();++i)
        {
            const char *name=resource_name;
            if(!m_providers[i].second.empty())
            {
                if(strncmp(resource_name,m_providers[i].second.c_str(),m_providers[i].second.size())!=0)
                    continue;

                name+=m_providers[i].second.size();
            }

            if(m_providers[i].first->has(name))
                return true;
        }

        return false;
    }

    if(m_ignore_case)
    {
        std::string res_str=fix_name(resource_name);
        std::transform(res_str.begin(),res_str.end(),res_str.begin(),::tolower);

        return m_cached_entries.find(res_str)!=m_cached_entries.end();
    }

    return m_cached_entries.find(fix_name(resource_name))!=m_cached_entries.end();
}

void RoxCompositeResourcesProvider::enableCache()
{
    m_mutex.lockWrite();

    if(m_cache_entries)
    {
        m_mutex.unlockWrite();
        return;
    }

    m_cache_entries=true;
    m_mutex.unlockWrite();

    rebuildCache();
}

void RoxCompositeResourcesProvider::rebuildCache()
{
    RoxMemory::RoxLockGuardWrite lock(m_mutex);

    if(!m_cache_entries)
        return;

    m_update_names=true;
    m_cached_entries.clear();
    for(int i=0;i<(int)m_providers.size();++i)
        cacheProvider(i);
}

int RoxCompositeResourcesProvider::getResourcesCount()
{
    if(m_update_names)
        updateNames();

    return (int)m_resource_names.size();
}

const char *RoxCompositeResourcesProvider::getResourceName(int idx)
{
    if(idx<0 || idx>=getResourcesCount())
        return 0;

    return m_resource_names[idx].c_str();
}

void RoxCompositeResourcesProvider::lock()
{
    RoxResourcesProvider::lock();

    if(m_update_names)
    {
        RoxResourcesProvider::unlock();
        m_mutex.lockWrite();
        if(m_update_names)
            updateNames();
        m_mutex.unlockWrite();
        lock();
    }
}

void RoxCompositeResourcesProvider::setIgnoreCase(bool ignore)
{
    m_mutex.lockWrite();

    if(ignore==m_ignore_case)
    {
        m_mutex.unlockWrite();
        return;
    }

    m_ignore_case=ignore;
    m_resource_names.clear();

    m_cached_entries.clear();
    if(m_cache_entries)
    {
        for(int i=0;i<(int)m_providers.size();++i)
            cacheProvider(i);
    }

    m_mutex.unlockWrite();

    if(ignore)
        rebuildCache();
}

void RoxCompositeResourcesProvider::cacheProvider(int idx)
{
    if(idx<0 || idx>=(int)m_providers.size())
        return;

    RoxResourcesProvider *provider=m_providers[idx].first;
    provider->lock();
    for(int i=0;i<provider->getResourcesCount();++i)
    {
        const char *name=provider->getResourceName(i);
        if(!name)
            continue;

        std::string name_str=fix_name(m_providers[idx].second+name);

        if(m_ignore_case)
            std::transform(name_str.begin(),name_str.end(),name_str.begin(),::tolower);

        entry &e=m_cached_entries[name_str];
        e.original_name.assign(name);
        e.prov_idx=idx;
    }
    provider->unlock();
}

void RoxCompositeResourcesProvider::updateNames()
{
    m_update_names=false;
    m_resource_names.clear();

    if(m_cache_entries)
    {
        for(entries_map::const_iterator it=m_cached_entries.begin();
            it!=m_cached_entries.end();++it)
            m_resource_names.push_back(it->first);
    }
    else
    {
        std::set<std::string> already_has;
        for(size_t i=0;i<m_providers.size();++i)
        {
            RoxResourcesProvider *provider=m_providers[i].first;
            provider->lock();
            for(int j=0;j<provider->getResourcesCount();++j)
            {
                const char *name=provider->getResourceName(j);
                if(!name)
                    continue;

                std::string name_str=fix_name(m_providers[i].second+name);
                
                if(already_has.find(name_str)!=already_has.end())
                    continue;
                
                m_resource_names.push_back(name_str);
                already_has.insert(name_str);
            }
            provider->unlock();
        }
    }
}

}
