//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "invalid_object.h"
#include "non_copyable.h"
#include <list>
#include <map>

namespace nya_memory
{

template<class t,size_t count> class lru: public non_copyable
{
protected:
    virtual bool on_access(const char *name,t& value) { return false; }
    virtual bool on_free(const char *name,t& value) { return true; }

public:
    t &access(const char *name)
    {
        if(!name)
            return invalid_object<t>();

        typename map::iterator it=m_map.find(name);
		if(it!=m_map.end())
        {
			m_list.splice(m_list.begin(),m_list,it->second);
			return it->second->second;
		}

		if(m_list.size()>=count)
        {
            typename list::iterator last=m_list.end();
			last--;
            on_free(last->first.c_str(),last->second);
			m_map.erase(last->first);
			m_list.pop_back();
		}

		m_list.push_front(entry(name,t()));
        if(!on_access(name,m_list.front().second))
        {
            m_list.pop_front();
            return invalid_object<t>();
        }

        m_map[name]=m_list.begin();
        return m_list.front().second;
    }

    void free(const char *name)
    {
        if(!name)
            return;

        typename map::iterator it=m_map.find(name);
        if(it==m_map.end())
            return;

        on_free(it->first.c_str(),it->second->second);
        m_list.erase(it->second);
        m_map.erase(it);
    }

    void clear()
    {
        for(typename list::iterator it=m_list.begin();it!=m_list.end();++it)
            on_free(it->first.c_str(),it->second);
        m_list.clear();
        m_map.clear();
    }

public: lru(){}

private:
    typedef std::pair<std::string,t> entry;
    typedef std::list<entry> list;
    typedef std::map<std::string,typename list::iterator> map;
    list m_list;
    map m_map;
};

}
