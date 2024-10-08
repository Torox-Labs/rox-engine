//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "config.h"
#include "attributes.h"
#include "string.h"
#include "stdlib.h"

#include <algorithm>

void bool_from_str(bool &b,const char *str)
{
    if(!str)
        return;

    if(strcasecmp(str,"true")==0 || strcmp(str,"1")==0)
        b=true;

    if(strcasecmp(str,"false")==0 || strcmp(str,"0")==0)
        b=false;
}

void float_from_str(float &f,const char *str)
{
    if(!str)
        return;

    f=atof(str);
}

void int_from_str(int &i,const char *str)
{
    if(!str)
        return;

    i=atoi(str);
}

bool config::load(nya_resources::resource_data *data)
{
    if(!data)
        return false;

    attribute_parser parser;
    bool result=parser.load(data);

    bool_from_str(wireframe_outline_enabled,parser.get_value("wireframe_outline_enabled"));
    bool_from_str(specular_enabled,parser.get_value("specular_enabled"));
    float_from_str(specular_level,parser.get_value("specular_level"));
    int_from_str(antialiasing,parser.get_value("antialiasing"));

    return result;
}

bool outline_ignore_list::should_ignore(const char *name)
{
    if(!name)
        return false;

    std::string name_str(name);
    std::transform(name_str.begin(),name_str.end(),name_str.begin(),::tolower);

    ignore_list::iterator it=m_list.find(name_str);
    if(it!=m_list.end())
        return true;

    return false;
}

bool outline_ignore_list::load(nya_resources::resource_data *data)
{
    if(!data || data->get_size()==0)
        return false;

    std::string content;
    content.resize(data->get_size());
    data->read_all(&content[0]);

    size_t prev=0;
    size_t endline=content.find("\n");
    while(endline!=std::string::npos)
    {
        std::string entry=content.substr(prev,endline-prev);
        std::transform(entry.begin(),entry.end(),entry.begin(),::tolower);
        m_list[entry]=0;
        prev=endline+1;
        endline=content.find("\n",prev);
    }

    if(prev<content.length()-1)
    {
        std::string entry=content.substr(prev,endline);
        std::transform(entry.begin(),entry.end(),entry.begin(),::tolower);
        m_list[entry]=0;
    }

    return false;
}

outline_ignore_list & get_outline_ignore()
{
    static outline_ignore_list list;
    return list;
}

config &get_config()
{
    static config cfg;
    return cfg;
}
