//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources/resources.h"
#include <vector>
#include <string>

struct smb_session;

namespace nya_resources
{

class smb_resources_provider: public resources_provider
{
public:
    bool open(const char *url,const char *user=0,const char *pwd=0);
    void close();

public:
    resource_data *access(const char *resource_name);
    bool has(const char *resource_name);

public:
    int get_resources_count();
    const char *get_resource_name(int idx);

public:
    smb_resources_provider(): m_session(0),m_tid(0) {}
    ~smb_resources_provider() { close(); }

private:
    bool list_dir(std::string subpath="");

private:
    std::vector<std::string> m_resource_names;
    smb_session *m_session;
    int m_tid;
    std::string m_path;
};

}
