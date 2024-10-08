//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "resources/resources.h"

#include <map>
#include <string>
#include <list>

class attribute
{
public:
    virtual const char *get_value(const char *key) { return 0; }
    virtual void debug_print() {}
};

class attribute_parser: public attribute
{
public:
    bool load(nya_resources::resource_data *data);
    const char *get_value(const char *key);
    void debug_print();

public:
    attribute_parser() {}
    attribute_parser(nya_resources::resource_data *data) { load(data); }

private:
    typedef std::map<std::string,std::string> attrib_map;
    typedef attrib_map::iterator attrib_iterator;
    attrib_map m_attributes;
};

class attribute_manager
{
public:
    attribute *get(const char *type,const char *name);

public:
    void load(nya_resources::resource_data *data);
    
public:
    void reset_iterator();
    const char *iterate_next();
    
public:
    void iterate_elements(const char *attrib_group);
    const char *iterate_next_element();

private:
    struct attributes
    {
        typedef std::map<std::string,attribute_parser> attrib_parsers_map;
        typedef attrib_parsers_map::iterator attrib_parsers_iterator;
        attrib_parsers_map parsers_map;
    };

    typedef std::map<std::string,attributes> attribs_map;
    typedef attribs_map::iterator attribs_iterator;
    attribs_map m_attributes;
    attribs_iterator m_iterator;

    attributes::attrib_parsers_iterator m_elements_iterator;
    attributes::attrib_parsers_iterator m_elements_end;
};

attribute_manager &get_attribute_manager();
