//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//requires: zlib http://www.zlib.net/

#include "zip_resources_provider.h"
#include "memory/tmp_buffer.h"
#include "memory/memory_reader.h"
#include "zlib.h"

//ToDo: log

namespace nya_resources
{

inline std::string fix_name(const char *name)
{
    if(!name)
        return std::string();

    std::string name_str(name);
    for(size_t i=0;i<name_str.size();++i)
    {
        if(name_str[i]=='\\')
            name_str[i]='/';
    }

    std::string out;
    char prev=0;
    for(size_t i=0;i<name_str.size();++i)
    {
        if(prev=='/' && name_str[i]=='/')
            continue;
        
        prev=name_str[i];
        out.push_back(prev);
    }
    
    return out;
}

bool zip_resources_provider::open_archive(const char *archive_name)
{
    close_archive();

    if(!archive_name || !archive_name[0])
        return false;

    return open_archive(nya_resources::get_resources_provider().access(archive_name));
}

bool zip_resources_provider::open_archive(nya_resources::resource_data *data)
{
    close_archive();

    if(!data)
        return false;

    typedef unsigned int uint;
    typedef unsigned short ushort;

    const size_t data_size=data->get_size();
    if(data_size<22)
        return false;

    size_t sign_offset=data_size-22;

    uint sign;
    if(!data->read_chunk(&sign,4,sign_offset))
        return false;

    const uint zip_sign=0x06054b50;
    if(sign!=zip_sign)
    {
        const size_t buf_size=65535;
        char buf[buf_size];
        size_t off=0;
        if(data_size<=buf_size)
        {
            if(!data->read_all(buf))
                return false;
        }
        else
        {
            off=data_size-buf_size;
            if(!data->read_chunk(buf,buf_size,off))
                return false;
        }

        while(--sign_offset>off)
        {
            if(memcmp(buf+sign_offset-off,&zip_sign,4)==0)
                break;
        }

        if(sign_offset<=off)
            return false;
    }

    struct { uint size,offset; } dir_size_offset;

    if(!data->read_chunk(&dir_size_offset,sizeof(dir_size_offset),sign_offset+12))
        return false;

    nya_memory::tmp_buffer_scoped dir_buf(dir_size_offset.size);

    if(!data->read_chunk(dir_buf.get_data(),dir_buf.get_size(),dir_size_offset.offset))
        return false;

    nya_memory::lock_guard_write lock(m_mutex);

    nya_memory::memory_reader reader(dir_buf.get_data(),dir_buf.get_size());
    while(reader.get_remained())
    {
        const uint pk_sign=0x02014b50;
        if(!reader.test(&pk_sign,sizeof(pk_sign)))
            break;

        zip_entry entry;

        reader.skip(6);
        entry.compression=reader.read<ushort>();
        reader.skip(4);
        entry.crc32=reader.read<uint>();
        entry.packed_size=reader.read<uint>();
        entry.unpacked_size=reader.read<uint>();

        const ushort file_name_len=reader.read<ushort>();
        const ushort extra_len=reader.read<ushort>();
        const ushort comment_len=reader.read<ushort>();

        reader.skip(8);
        entry.offset=reader.read<uint>();

        entry.name=fix_name(std::string((const char *)reader.get_data(),file_name_len).c_str());
        reader.skip(file_name_len+extra_len+comment_len);

        if(entry.unpacked_size==0)
        {
            if(entry.name.empty())
                continue;

            if(entry.name[entry.name.size()-1]=='/')
                continue;
        }

        m_entries.push_back(entry);
    }

    m_res=data;
    return true;
}

void zip_resources_provider::close_archive()
{
    nya_memory::lock_guard_write lock(m_mutex);

    if(m_res)
        m_res->release();

    m_res=0;
    m_entries.clear();
}

namespace
{

class zip_resource: public resource_data
{
public:
    size_t get_size() { return m_unpacked_size; }

    bool read_all(void*data)
    {
        if(m_compression==0)
            return m_res?m_res->read_chunk(data,m_unpacked_size,m_offset):false;

        if(m_data.get_size()>0)
            return m_data.copy_to(data,m_data.get_size());

        return unpack_to(data);
    }

    bool read_chunk(void *data,size_t size,size_t offset)
    {
        if(m_compression==0)
        {
            if(size+offset>m_unpacked_size)
            {
                log()<<"unable to read file data chunk: invalid size\n";
                return false;
            }

            return m_res?m_res->read_chunk(data,size,m_offset+offset):false;
        }

        m_data.allocate(m_unpacked_size);
        if(!unpack_to(m_data.get_data()))
        {
            m_data.free();
            return false;
        }

        return m_data.copy_to(data,size,offset);
    }

    void release() { m_data.free(); delete this; }

public:
    zip_resource(nya_resources::resource_data *res,unsigned int compression,unsigned int offset,unsigned int packed_size,unsigned int unpacked_size):
                 m_res(res),m_compression(compression),m_offset(offset),m_packed_size(packed_size),m_unpacked_size(unpacked_size)
    {
        if(!m_res)
            return;

        struct { unsigned short file_name_len,extra_field_len; } header;
        if(m_res->read_chunk(&header,4,m_offset+26))
            m_offset+=header.file_name_len+header.extra_field_len+30;
    }

private:
    bool unpack_to(void *data)
    {
        if(!data || !m_res)
            return false;

        if(m_compression!=8)
            return false;

        nya_memory::tmp_buffer_scoped packed_buf(m_packed_size);
        m_res->read_chunk(packed_buf.get_data(),m_packed_size,m_offset);

        z_stream infstream;
        infstream.zalloc=Z_NULL;
        infstream.zfree=Z_NULL;
        infstream.opaque=Z_NULL;
        infstream.avail_in=(uInt)packed_buf.get_size();
        infstream.next_in=(Bytef *)packed_buf.get_data();
        infstream.avail_out=(uInt)m_unpacked_size;
        infstream.next_out=(Bytef *)data;

        inflateInit2(&infstream,-MAX_WBITS);
        const int err=inflate(&infstream,Z_NO_FLUSH);
        if(err!=Z_OK && err!=Z_STREAM_END)
            return false;

        inflateEnd(&infstream);
        return true;
    }

private:
    nya_resources::resource_data *m_res;
    nya_memory::tmp_buffer_ref m_data;
    unsigned int m_compression,m_offset,m_packed_size,m_unpacked_size;
};

}

resource_data *zip_resources_provider::access(const char *resource_name)
{
    std::string name=fix_name(resource_name);
    if(name.empty())
        return 0;

    nya_memory::lock_guard_read lock(m_mutex);

    for(int i=0;i<(int)m_entries.size();++i)
    {
        const zip_entry &e=m_entries[i];
        if(e.name==name)
            return new zip_resource(m_res,e.compression,e.offset,e.packed_size,e.unpacked_size);
    }

    return 0;
}

bool zip_resources_provider::has(const char *resource_name)
{
    std::string name=fix_name(resource_name);
    if(name.empty())
        return false;

    nya_memory::lock_guard_read lock(m_mutex);

    for(int i=0;i<(int)m_entries.size();++i)
    {
        if(m_entries[i].name==name)
            return true;
    }

    return false;
}

int zip_resources_provider::get_resources_count() { return (int)m_entries.size(); }

const char *zip_resources_provider::get_resource_name(int idx)
{
    if(idx<0 || idx>=(int)m_entries.size())
        return 0;

    return m_entries[idx].name.c_str();
}

int zip_resources_provider::get_resource_idx(const char *resource_name)
{
    std::string name=fix_name(resource_name);
    if(name.empty())
        return -1;

    for(int i=0;i<(int)m_entries.size();++i)
    {
        const zip_entry &e=m_entries[i];
        if(e.name==name)
            return i;
    }
    return -1;
}

unsigned int zip_resources_provider::get_resource_size(int idx, bool packed)
{
    if(idx<0 || idx>=(int)m_entries.size())
        return 0;

    return packed ? m_entries[idx].packed_size : m_entries[idx].unpacked_size;
}

unsigned int zip_resources_provider::get_resource_crc32(int idx)
{
    if(idx<0 || idx>=(int)m_entries.size())
        return 0;

    return m_entries[idx].crc32;
}

unsigned int zip_resources_provider::get_crc32(const void *data,size_t size)
{
    return (unsigned int)crc32(0L,(const Bytef *)data,(uInt)size);
}

}
