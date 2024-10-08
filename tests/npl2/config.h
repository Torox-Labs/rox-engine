//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources/resources.h"

#include <string>
#include <map>

struct config
{
    bool wireframe_outline_enabled;
    bool specular_enabled;
    float specular_level;
    int antialiasing;

public:
    config(): wireframe_outline_enabled(true),specular_enabled(true),
              specular_level(0.06),antialiasing(4) {}

public:
    bool load(nya_resources::resource_data *data);
};

class outline_ignore_list
{
public:
    bool should_ignore(const char *name);

public:
    bool load(nya_resources::resource_data *data);

private:
    typedef std::map<std::string,bool> ignore_list;
    ignore_list m_list;
};

config &get_config();
outline_ignore_list & get_outline_ignore();
