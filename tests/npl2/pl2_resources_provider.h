//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources/resources.h"
#include <vector>

namespace nya_resources
{

class pl2_resources_provider: public resources_provider
{
public:
    bool open_archieve(resource_data *archieve_data);

    bool open_archieve(const char *archieve_name,resources_provider &provider)
    {
        resource_data *archieve_data=provider.access(archieve_name);
        if(!archieve_data)
            return false;

        return open_archieve(archieve_data);
    }

    bool open_archieve(const char *archieve_name)
    {
        return open_archieve(archieve_name,get_resources_provider());
    }

    void close_archieve();

    resource_data *access_attribute();

public:
    int get_resources_count() { return (int)m_entries.size(); }
    const char *get_resource_name(int idx);

public:
    resource_data *access(const char *resource_name);
    bool has(const char *resource_name);

public:
    pl2_resources_provider(): m_archieve_data(0) {}
    ~pl2_resources_provider() { close_archieve(); }

private:
    resource_data *m_archieve_data;

    struct entry
    {
        std::string name;
        unsigned int offset;
        unsigned int packed_size;
        unsigned int size;
    };

    resource_data *access(const entry &e);

    std::vector<entry> m_entries;
    entry m_attribute;
};

}
