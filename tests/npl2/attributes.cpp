//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "attributes.h"
#include "memory/tmp_buffer.h"

bool attribute_parser::load(nya_resources::resource_data *data)
{
    m_attributes.clear();

    if(!data)
    {
        nya_log::log()<<"Unable to load attribute: invalid data\n";
        return false;
    }

    size_t data_size=data->get_size();
    if(!data_size)
    {
        nya_log::log()<<"Unable to load attribute: invalid data size\n";
        return false;
    }

    nya_memory::tmp_buffer_scoped data_buf(data_size);
    data->read_all(data_buf.get_data());

    std::string key;
    std::string value;

    bool parse_key=true;
    bool parse_comment=false;
    for(size_t i=0;i<data_size;++i)
    {
        const char c = *(const char*)data_buf.get_data(i);

        if(c=='#')
        {
            parse_comment=true;
            parse_key=true;
            continue;
        }

        if(c=='\r' || c=='\n')
        {
            if(parse_comment||parse_key)
            {
                parse_comment=false;
                parse_key=true;
                key.clear();
                value.clear();
                continue;
            }

            if(key.empty() || value.empty())
                continue;

            std::pair<attrib_iterator,bool> ir=
            m_attributes.insert(std::make_pair(key,value));
            if(!ir.second)
                nya_log::log()<<"Attribute parser warning: dublicate attribute"
                                  <<key.c_str()<<"\n";

            if(key=="TYPE" && value[0]=='A')
            {
                if(value.size()>6 && value[1]=='R'
                    && value[2]=='C'&& value[3]=='H')
                return false;
            }

            key.clear();
            value.clear();
            parse_key=true;
            continue;
        }

        if(parse_comment)
            continue;

        if(c=='=')
        {
            parse_key=false;
            continue;
        }

        if(parse_key)
            key.push_back(c);
        else
            value.push_back(c);
    }

    if(!key.empty()&&!value.empty())
    {
        std::pair<attrib_iterator,bool> ir=
        m_attributes.insert(std::make_pair(key,value));
        if(!ir.second)
            nya_log::log()<<"Attribute parser warning: dublicate attribute"
                              <<key.c_str()<<"\n";
    }

    return true;
}

const char *attribute_parser::get_value(const char *key)
{
    if(!key)
        return 0;

    attrib_iterator it=m_attributes.find(key);
    if(it==m_attributes.end())
        return 0;

    return it->second.c_str();
}

void attribute_parser::debug_print()
{
    for(attrib_iterator it=m_attributes.begin();it!=m_attributes.end();
        ++it)
    {
        std::string safe_prnt=it->second;
        for(size_t i=0;i<safe_prnt.length();++i)
        {
            char &c = safe_prnt[i];
            if(c<32||c>127)
                c='?';
        }

        nya_log::log()<<"<"<<it->first.c_str()<<"> = <"<<safe_prnt.c_str()<<">\n";
    }
}

attribute *attribute_manager::get(const char *type,const char *name)
{
    if(!type||!name)
    {
        nya_log::log()<<"Unable to get attribute: invalid name or type\n";
        return 0;
    }

    attribs_iterator it=m_attributes.find(type);
    if(it==m_attributes.end())
    {
        nya_log::log()<<"Unable to get attribute "<<name<<": type not found\n";
        return 0;
    }

    attributes::attrib_parsers_map &a=it->second.parsers_map;
    attributes::attrib_parsers_iterator it2=a.find(name);
    if(it2==a.end())
    {
        //nya_log::get_log()<<"Unable to get attribute "<<name<<": not found\n";
        return 0;
    }

    return &it2->second;
}

void attribute_manager::load(nya_resources::resource_data *data)
{
    if(!data)
    {
        nya_log::log()<<"Unable to load attribute: invalid data\n";
        return;
    }

    attribute_parser p(data);
    const char *type=p.get_value("TYPE");
    if(!type)
    {
        nya_log::log()<<"Unable to load attribute: invalid type\n";
        return;
    }

    std::string type_str(type);
    if(type_str=="ARCHIVE")
        return;

    const char *name=p.get_value("NAME");
    if(!name)
    {
        nya_log::log()<<"Unable to load attribute ("<<type<<"): invalid name\n";
        return;
    }

    //printf("Attrib: <%s|%s>\n",type, name);

    std::pair<attribs_iterator,bool> it=
    m_attributes.insert(std::make_pair(type_str,attributes()));

    std::pair<attributes::attrib_parsers_iterator,bool> it2=
    it.first->second.parsers_map.insert(std::make_pair(name,p));
    if(!it2.second)
    {
        nya_log::log()<<"Doublicate attribute "<<name<<" of type "<<type<<"\n";
    }
}

void attribute_manager::reset_iterator()
{
    m_iterator=m_attributes.begin();
}

const char *attribute_manager::iterate_next()
{
    if(m_iterator==m_attributes.end())
        return 0;

    const char *name=m_iterator->first.c_str();

    ++m_iterator;

    return name;
}

void attribute_manager::iterate_elements(const char *attrib_group)
{
    if(!attrib_group)
    {
        nya_log::log()<<"Unable to iterate attribute elements: invalid attribute group\n";
        return;
    }

    attribs_iterator it=m_attributes.find(attrib_group);
    if(it==m_attributes.end())
    {
        nya_log::log()<<"Unable to iterate attribute elements "<<attrib_group<<": type not found\n";
        return;
    }

    attributes::attrib_parsers_map &a=it->second.parsers_map;

    m_elements_iterator=a.begin();
    m_elements_end=a.end();
}

const char *attribute_manager::iterate_next_element()
{
    if(m_elements_iterator==m_elements_end)
        return 0;

    const char *name=m_elements_iterator->first.c_str();

    ++m_elements_iterator;

    return name;
}

attribute_manager &get_attribute_manager()
{
    static attribute_manager manager;
    return manager;
}

