//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources.h"
#include <string>
#include <vector>

namespace nya_resources
{

class file_resources_provider: public resources_provider
{
public:
    resource_data *access(const char *resource_name);
    bool has(const char *resource_name);

public:
    bool set_folder(const char *folder,bool recursive=true,bool ignore_nonexistent=false);

public:
    int get_resources_count();
    const char *get_resource_name(int idx);

public:
    virtual void lock();

public:
    file_resources_provider(const char *folder="") { set_folder(folder); }

private:
    void enumerate_folder(const char *folder_name);
    void update_names();

private:
    std::string m_path;
    bool m_recursive;
    bool m_update_names;
    std::vector<std::string> m_resource_names;
};

}
