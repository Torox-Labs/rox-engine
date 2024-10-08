//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources.h"
#include <vector>
#include <string>

namespace nya_resources
{

class memory_resources_provider: public resources_provider
{
public:
    resource_data *access(const char *resource_name);
    bool has(const char *resource_name);

public:
    bool add(const char *name,const void *data,size_t size);
    bool remove(const char *name);

public:
    int get_resources_count();
    const char *get_resource_name(int idx);

private:
    struct entry { std::string name; const char *data; size_t size; };
    std::vector<entry> m_entries;
};

}
