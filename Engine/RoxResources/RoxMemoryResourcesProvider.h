//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxResources.h"
#include <vector>
#include <string>

namespace RoxResources
{

class RoxMemoryResourcesProvider: public RoxResourcesProvider
{
public:
    RoxResourceData *access(const char *resource_name);
    bool has(const char *resource_name);

public:
    bool add(const char *name,const void *data,size_t size);
    bool remove(const char *name);

public:
    int getResourcesCount();
    const char *getResourceName(int idx);

private:
    struct entry { std::string name; const char *data; size_t size; };
    std::vector<entry> m_entries;
};

}
