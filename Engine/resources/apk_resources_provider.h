//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources.h"
#include <vector>
#include <string>

class AAssetManager;

namespace nya_resources
{

class apk_resources_provider: public resources_provider
{
public:
    resource_data *access(const char *resource_name);
    bool has(const char *resource_name);

public:
    bool set_folder(const char *folder);

public:
    int get_resources_count();
    const char *get_resource_name(int idx);

public:
    virtual void lock();

public:
    apk_resources_provider(const char *folder="") { set_folder(folder); }

public:
    static void set_asset_manager(AAssetManager *mgr);

private:
    void update_names();

private:
    std::vector<std::string> m_names;
    bool m_update_names;
    std::string m_folder;
};

}
