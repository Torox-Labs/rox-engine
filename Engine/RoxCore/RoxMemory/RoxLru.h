//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxInvalidObject.h"
#include "RoxNonCopyable.h"
#include <list>
#include <map>
#include <string>

namespace RoxMemory
{

template<class t,size_t count> class RoxLru: public RoxNonCopyable
{
protected:
    virtual bool onAccess(const char *name,t& value) { return false; }
    virtual bool onFree(const char *name,t& value) { return true; }

public:
    t &access(const char *name)
    {
        if(!name)
            return invalidObject<t>();

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
            onFree(last->first.c_str(),last->second);
			m_map.erase(last->first);
			m_list.pop_back();
		}

		m_list.push_front(entry(name,t()));
        if(!onAccess(name,m_list.front().second))
        {
            m_list.pop_front();
            return invalidObject<t>();
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

        onFree(it->first.c_str(),it->second->second);
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

public:
	RoxLru(){}

private:
    typedef std::pair<std::string,t> entry;
    typedef std::list<entry> list;
    typedef std::map<std::string,typename list::iterator> map;
    list m_list;
    map m_map;
};

}
