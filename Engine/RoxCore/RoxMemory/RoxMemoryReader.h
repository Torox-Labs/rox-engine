//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <cstddef>
#include <cstring>
#include <string>

namespace RoxMemory
{

class RoxMemoryReader
{
public:
    template <typename t> t read()
    {
        const size_t size=sizeof(t);
        if(size>m_size-m_offset)
        {
            m_offset=m_size;
            return t();
        }

        t a;
        memcpy(&a,m_data+m_offset,size);
        m_offset+=size;
        return a;
    }

    std::string readString() { return readString<unsigned short>(); }
    template <typename t> std::string readString()
    {
        const t size=read<t>();
        const char *str=(const char *)getData();
        if(!checkRemained(size) || !size)
            return "";

        m_offset+=size;
        return str?std::string(str,size):"";
    }

    bool test(const void*data,size_t size)
    {
        if(size>m_size-m_offset)
        {
            //m_offset=m_size;
            return false;
        }

        if(memcmp(m_data+m_offset,data,size)!=0)
        {
            //m_offset+=size;
            return false;
        }

        m_offset+=size;
        return true;
    }

    bool checkRemained(size_t size) const { return size<=m_size-m_offset; }

    bool seek(size_t offset)
    {
        if(offset>=m_size)
        {
            m_offset=m_size;
            return false;
        }

        m_offset=offset;
        return true;
    }

    bool skip(size_t offset)
    {
        m_offset+=offset;
        if(m_offset<offset || m_offset > m_size) //overflow
            m_offset=m_size;

        return m_offset<m_size;
    }

    bool rewind(size_t offset)
    {
        if(offset>m_offset)
        {
            m_offset=0;
            return false;
        }

        m_offset-=offset;
        return true;
    }

    size_t getOffset() const { return m_offset; }

    size_t getRemained() const
    {
        if(m_offset>=m_size)
            return 0;

        return m_size-m_offset;
    }

    const void *getData()  const
    {
        if(m_offset>=m_size)
            return 0;

        return m_data+m_offset;
    }

    RoxMemoryReader(const void *data,size_t size)
    {
        if(data)
        {
            m_data=(char*)data;
            m_size=size;
        }
        else
        {
            m_data=0;
            m_size=0;
        }

        m_offset=0;
    }

private:
    const char *m_data;
    size_t m_size;
    size_t m_offset;
};

}
