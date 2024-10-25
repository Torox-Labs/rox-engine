//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources/resources.h"
#include <vector>
#include <string>

namespace nya_resources
{

class ftp_resources_provider: public resources_provider
{
public:
    bool open(const char *url,const char *userpwd = 0);
    void close();

public:
    resource_data *access(const char *resource_name);
    bool has(const char *resource_name);

public:
    int get_resources_count();
    const char *get_resource_name(int idx);

public:
    ftp_resources_provider(): m_curl(0) {}
    ~ftp_resources_provider() { close(); }

private:
    bool get_entries(const char *folder);

private:
    void *m_curl;
    std::vector<std::string> m_entries;
    std::string m_base_url;
};

}
