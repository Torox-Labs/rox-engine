//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>
#include <string.h>

namespace nya_scene
{

class tags
{
public:
    int get_count() const { return int(m_inds.size()); }

    const char *get(int idx) const
    {
        if(idx<0 || idx>=get_count())
            return 0;

        return &m_buf[m_inds[idx]];
    }

    bool has(const char *tag) const
    {
        if(!tag)
            return false;

        for(int i=0;i<get_count();++i)
        {
            if(strcmp(&m_buf[m_inds[i]],tag)==0)
                return true;
        }

        return false;
    }

    void add(const char *tag)
    {
        if(!tag || !*tag || has(tag))
            return;

        m_inds.push_back((unsigned short)m_buf.size());
        const char *c=tag;
        do m_buf.push_back(*c); while(*c++);
    }

    void add(const tags &t)
    {
        for(int i=0;i<t.get_count();++i)
            add(t.get(i));
    }

public:
    tags() {}
    tags(const char *str,char delimeter=',')
    {
        if(!str || !*str)
            return;

        m_inds.push_back(0);
        char c; unsigned short idx=0;
        while(++idx,c=*str++)
        {
            if(c==delimeter)
            {
                m_inds.push_back(idx);
                m_buf.push_back(0);
            }
            else
                m_buf.push_back(c);
        }
        m_buf.push_back(0);
    }

private:
    std::vector<char> m_buf;
    std::vector<unsigned short> m_inds;
};

}
