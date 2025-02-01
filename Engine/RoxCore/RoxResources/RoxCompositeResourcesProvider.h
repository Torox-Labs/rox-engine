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
