//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <cstddef>
#include <string.h>
#include <string>

namespace nya_memory
{

class memory_writer
{
public:
    template<typename t> bool write(const t&v) { return write(&v,sizeof(t)); }
    bool write_short(short v) { return write(&v,sizeof(v)); }
    bool write_int(int v) { return write(&v,sizeof(v)); }
    bool write_uint(unsigned int v) { return write(&v,sizeof(v)); }
    bool write_ushort(unsigned short v) { return write(&v,sizeof(v)); }
    bool write_ubyte(unsigned char v) { return write(&v,sizeof(v)); }
    bool write_float(float v) { return write(&v,sizeof(v)); }

    bool write_string(const std::string &s) { return write_string<unsigned short>(s); }
    template<typename t> bool write_string(const std::string &s)
    {
        t len=(t)s.length();
        return write(&len,sizeof(t)) && write(s.c_str(),len);
    }

    bool write(const void *data,size_t size)
    {
        if(!data || !size)
            return false;

        if(size>m_size-m_offset)
        {
            if(m_data)
                return false;

            m_size=m_offset+size;
        }

        if(m_data)
            memcpy(m_data+m_offset,data,size);

        m_offset+=size;
        return true;
    }

    bool seek(size_t offset)
    {
        if(m_data && offset>=m_size)
        {
            m_offset=m_size;
            return false;
        }

        m_offset=offset;
        return true;
    }

    size_t get_offset() const { return m_offset; }
    size_t get_size() const { return m_size; }

public:
    memory_writer(void *data,size_t size) //only counts size if data==0
    {
        m_data=(char*)data;
        m_size=size;
        m_offset=0;
    }

private:
    char *m_data;
    size_t m_size;
    size_t m_offset;
};

}
