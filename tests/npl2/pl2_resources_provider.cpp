//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "pl2_resources_provider.h"
#include "memory/pool.h"
#include "memory/tmp_buffer.h"
#include <string>
#include <memory.h>

namespace nya_resources
{

class pl2_resource: public resource_data
{
public:
    size_t get_size() { return m_data.get_size(); }
    bool read_all(void*data) { return m_data.copy_to(data,m_data.get_size()); }
    bool read_chunk(void *data,size_t size,size_t offset) { return m_data.copy_to(data,size,offset); }

public:
    bool decompress(const void*data,size_t packed_size,size_t size);
    void release() { m_data.free(); }

private:
    nya_memory::tmp_buffer_ref m_data;
};

namespace { nya_memory::pool<pl2_resource,8> pl2_resources; }

bool pl2_resources_provider::open_archieve(resource_data *archieve_data)
{
    if(m_archieve_data)
        close_archieve();

    if(!archieve_data)
        return false;

    m_archieve_data=archieve_data;

    typedef unsigned int uint;

    struct pl2_entry
    {
        char name[32];
        uint offset;
        uint packed_size;
        uint size;
        uint align_bytes;
    };

    const size_t data_size = archieve_data->get_size();
    const size_t header_size = sizeof(uint)*4;
    const size_t entry_size = sizeof(pl2_entry);

    if(data_size<header_size+entry_size)
        return false;

    pl2_entry first_entry;
    archieve_data->read_chunk(&first_entry,entry_size,header_size);

    m_attribute.name.assign(first_entry.name);
    m_attribute.size=first_entry.size;
    m_attribute.offset=first_entry.offset;
    m_attribute.packed_size=first_entry.packed_size;

    uint ecount=(first_entry.offset-16)/entry_size;
    if(ecount<=1)
        return true;

    --ecount;

    const uint last_entries_size=ecount*entry_size;
    if(data_size<header_size+entry_size+last_entries_size)
        return false;

    nya_memory::tmp_buffer_scoped tmp(last_entries_size);
    archieve_data->read_chunk(tmp.get_data(),last_entries_size,header_size+entry_size);

    pl2_entry *last_entries=(pl2_entry *)tmp.get_data();
    m_entries.resize(ecount);
    for(int i=0;i<ecount;++i)
    {
        const pl2_entry &from = last_entries[i];

        entry &to=m_entries[i];
        to.name.assign(from.name);
        to.size=from.size;
        to.offset=from.offset;
        to.packed_size=from.packed_size;
    }

    return true;
}

void pl2_resources_provider::close_archieve()
{
    if(m_archieve_data)
        m_archieve_data->release();

    m_entries.clear();
    m_attribute=entry();
    m_archieve_data=0;
}

const char *pl2_resources_provider::get_resource_name(int idx)
{
    if(idx<0 || idx>=(int)m_entries.size())
        return 0;

    return m_entries[idx].name.c_str();
}

resource_data *pl2_resources_provider::access(const char *resource_name)
{
    if(!resource_name)
    {
        log()<<"unable to access archieve entry: invalid name\n";
        return 0;
    }

    for(int i=0;i<(int)m_entries.size();++i)
    {
        if(m_entries[i].name==resource_name)
            return access(m_entries[i]);
    }

    log()<<"unable to access archieve entry: not found\n";
    return 0;
}

bool pl2_resources_provider::has(const char *resource_name)
{
    if(!resource_name)
        return false;

    for(int i=0;i<(int)m_entries.size();++i)
    {
        if(m_entries[i].name==resource_name)
            return true;
    }

    return false;
}

resource_data *pl2_resources_provider::access_attribute()
{
    if(m_attribute.name.empty())
        return 0;

    return access(m_attribute);
}

resource_data *pl2_resources_provider::access(const entry &e)
{
    if(!m_archieve_data || !e.packed_size || !e.size)
    {
        log()<<"unable to access archieve entry: invalid size\n";
        return 0;
    }

    nya_memory::tmp_buffer_scoped packed(e.packed_size);

    if(!m_archieve_data->read_chunk(packed.get_data(),e.packed_size,e.offset))
    {
        log()<<"unable to access archieve entry: invalid data\n";
        return 0;
    }

    pl2_resource *data = pl2_resources.allocate();
    if(!data->decompress(packed.get_data(),e.packed_size,e.size))
    {
        pl2_resources.free(data);
        log()<<"unable to access archieve entry: decompress failed\n";
        return 0;
    }

    return data;
}

bool pl2_resource::decompress(const void*data,size_t packed_size,size_t size)
{
    if(!data || !packed_size || !size)
        return false;

    typedef unsigned char uchar;

    m_data.allocate(size);
    const uchar *src=(const uchar*)data;
    uchar *dst=(uchar*)m_data.get_data();
    if(!dst)
        return false;

    const size_t lzbuf_size=4096;
    uchar lzbuf[lzbuf_size]={0};

    uchar flags=src[0], mask=1;
    const uchar offset_mask=0xf0;
    const uchar length_mask=0x0f;

    size_t src_pos=0, dst_pos=0;
    while(++src_pos < packed_size)
    {
        if (flags & mask)
        {
            dst[dst_pos]=lzbuf[dst_pos%lzbuf_size]=src[src_pos];
            ++dst_pos;
        }
        else
        {
            const uchar b[]={src[src_pos],src[src_pos+1]};
            ++src_pos;
            size_t offset=(((b[1] & offset_mask)<<4)|b[0])+18;
            for(size_t i=0;i<(b[1] & length_mask)+3;++i,++offset,++dst_pos)
                dst[dst_pos]=lzbuf[dst_pos%lzbuf_size]=lzbuf[offset%lzbuf_size];
        }

        if(!(mask += mask))
        {
            mask=1;
            if(++src_pos < packed_size)
                flags=src[src_pos];
        }
    }

    return true;
}

}
