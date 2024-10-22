//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxResources.h"

#include <map>
#include <string>
#include <vector>

namespace RoxResources
{

class RoxCompositeResourcesProvider: public RoxResourcesProvider
{
public:
    void addProvider(RoxResourcesProvider *provider,const char *folder=0);
    void removeProviders();
    void enableCache();
    void rebuildCache();
    void setIgnoreCase(bool ignore); //enables cache if true

public:
    RoxResourceData*access(const char *resource_name);
    bool has(const char *resource_name);

public:
    int getResourcesCount();
    const char *getResourceName(int idx);

public:
    virtual void lock();

public:
    RoxCompositeResourcesProvider(): m_ignore_case(false),m_cache_entries(false),m_update_names(false) {}

private:
    void cacheProvider(int idx);
    void updateNames();

private:
    std::vector<std::pair<RoxResourcesProvider*,std::string> > m_providers;
    std::vector<std::string> m_resource_names;

    struct entry
    {
        std::string original_name;
        int prov_idx;
    };

    typedef std::map<std::string,entry> entries_map;
    entries_map m_cached_entries;

    bool m_ignore_case;
    bool m_cache_entries;
    bool m_update_names;
};

}
