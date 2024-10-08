//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources/resources.h"
#include <vector>
#include <string>

namespace nya_resources
{

class zip_resources_provider: public resources_provider
{
public:
    bool open_archive(const char *archive_name);
    bool open_archive(resource_data *data);
    void close_archive();

public:
    resource_data *access(const char *resource_name);
    bool has(const char *resource_name);

public:
    int get_resources_count();
    const char *get_resource_name(int idx);

public:
    int get_resource_idx(const char *resource_name);
    unsigned int get_resource_size(int idx,bool packed=false);
    unsigned int get_resource_crc32(int idx);
    static unsigned int get_crc32(const void *data,size_t size);

public:
    zip_resources_provider(const char *archive_name=""): m_res(0) { open_archive(archive_name); }
    ~zip_resources_provider() { close_archive(); }

private:
    nya_resources::resource_data *m_res;

    struct zip_entry
    {
        std::string name;
        unsigned int compression;
        unsigned int offset;
        unsigned int packed_size;
        unsigned int unpacked_size;
        unsigned int crc32;
    };

    std::vector<zip_entry> m_entries;
};

}
