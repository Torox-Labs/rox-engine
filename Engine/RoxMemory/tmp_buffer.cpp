//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "tmp_buffer.h"
#include "align_alloc.h"
#include "mutex.h"
#include "memory.h"
#include <memory.h>
#include <string.h>
#include <list>

namespace nya_memory
{

class tmp_buffer
{
private:
    void allocate(size_t size)
    {
        if(size>m_alloc_size)
        {
            if(m_allocate_log_enabled)
                log()<<"tmp buf resized from "<<m_alloc_size<<" to "<<size<<", ";

            if(m_data)
                align_free(m_data);

            m_data=(char *)align_alloc(size,16);
            m_alloc_size=size;

            if(m_allocate_log_enabled)
                log()<<get_total_size()<<" in "<<m_buffers.size()<<" buffers total\n";
        }

        m_size=size;
        m_used=true;
    }

    bool is_used() const { return m_used; }
    size_t get_actual_size() const { return m_alloc_size; }

public:
    size_t get_size() const { return m_size; }

    void free()
    {
        lock_guard guard(m_mutex);

        m_size=0;
        m_used=false;
    }

    void *get_data(size_t offset)
    {
        if(offset>=m_size)
            return 0;

        return m_data+offset;
    }

    const void *get_data(size_t offset) const
    {
        if(offset>=m_size)
            return 0;

        return m_data+offset;
    }

    bool copy_to(void *data,size_t size,size_t offset) const
    {
        if(size+offset>m_size)
            return false;

        memcpy(data,m_data+offset,size);
        return true;
    }

    bool copy_from(const void *data,size_t size,size_t offset)
    {
        if(size+offset>m_size)
            return false;

        memcpy(m_data+offset,data,size);
        return true;
    }

    static tmp_buffer *allocate_new(size_t size)
    {
        m_mutex.lock();

        tmp_buffer* min_suit_buf=0;
        tmp_buffer* max_buf=0;

        for(buffers_list::iterator it=m_buffers.begin();it!=m_buffers.end();++it)
        {
            tmp_buffer &buffer = *it;
            if(buffer.is_used())
                continue;

            if(buffer.get_actual_size()>=size && (!min_suit_buf  || buffer.get_actual_size()< min_suit_buf->get_actual_size()))
                min_suit_buf=&buffer;

            if(max_buf)
            {
                if(buffer.get_actual_size() > max_buf->get_actual_size())
                    max_buf=&buffer;
            }
            else
                max_buf=&buffer;
        }

        if(min_suit_buf)
        {
            m_mutex.unlock();
            min_suit_buf->allocate(size);
            return min_suit_buf;
        }

        if(max_buf)
        {
            m_mutex.unlock();
            max_buf->allocate(size);
            return max_buf;
        }

        m_buffers.push_back(tmp_buffer());
        tmp_buffer* result=&m_buffers.back();
        m_mutex.unlock();
        m_buffers.back().allocate(size);

        if(m_allocate_log_enabled) log()<<"new tmp buf allocated ("<<m_buffers.size()<<" total)\n";

        return result;
    }

    static void force_free()
    {
        lock_guard guard(m_mutex);

        for(buffers_list::iterator it=m_buffers.begin();it!=m_buffers.end();++it)
        {
            tmp_buffer &buffer = *it;
            if(buffer.is_used())
                continue;

            if(!buffer.m_data)
                continue;

            align_free(buffer.m_data);
            buffer.m_data=0;
            buffer.m_alloc_size=buffer.m_size=0;
        }
    }

    static size_t get_total_size()
    {
        lock_guard guard(m_mutex);

        size_t size=0;
        for(buffers_list::iterator it=m_buffers.begin();it!=m_buffers.end();++it)
        {
            tmp_buffer &buffer = *it;
            size+=buffer.m_alloc_size;
        }

        return size;
    }

    static void enable_alloc_log(bool enable) { m_allocate_log_enabled=enable; }

    tmp_buffer(): m_used(false),m_data(0),m_size(0),m_alloc_size(0) {}

private:
    bool m_used;
    char *m_data;
    size_t m_size;
    size_t m_alloc_size;

private:
    typedef std::list<tmp_buffer> buffers_list;
    static buffers_list m_buffers;
    static bool m_allocate_log_enabled;
    static mutex m_mutex;
};

tmp_buffer::buffers_list tmp_buffer::m_buffers;
bool tmp_buffer::m_allocate_log_enabled=false;
mutex tmp_buffer::m_mutex;

void *tmp_buffer_ref::get_data(size_t offset) const
{
    if(!m_buf)
        return 0;

    return m_buf->get_data(offset);
}

size_t tmp_buffer_ref::get_size() const
{
    if(!m_buf)
        return 0;

    return m_buf->get_size();
}

bool tmp_buffer_ref::copy_from(const void*data,size_t size,size_t offset)
{
    if(!m_buf)
        return false;

    return m_buf->copy_from(data,size,offset);
}

bool tmp_buffer_ref::copy_to(void*data,size_t size,size_t offset) const
{
    if(!m_buf)
        return false;

    return m_buf->copy_to(data,size,offset);
}

void tmp_buffer_ref::allocate(size_t size)
{
    free();

    if(!size)
        return;

    m_buf=tmp_buffer::allocate_new(size);
}

void tmp_buffer_ref::free()
{
    if(!m_buf)
        return;

    m_buf->free();
    m_buf=0;
}

void *tmp_buffer_scoped::get_data(size_t offset) const { return m_buf?m_buf->get_data(offset):0; }
size_t tmp_buffer_scoped::get_size() const { return m_buf?m_buf->get_size():0;}
bool tmp_buffer_scoped::copy_from(const void*data,size_t size,size_t offset) { return m_buf?m_buf->copy_from(data,size,offset):false; }
bool tmp_buffer_scoped::copy_to(void*data,size_t size,size_t offset) const { return m_buf?m_buf->copy_to(data,size,offset):false; }

tmp_buffer_scoped::tmp_buffer_scoped(size_t size): m_buf(tmp_buffer::allocate_new(size)) {}
void tmp_buffer_scoped::free() { if(m_buf) m_buf->free(); m_buf=0; }
tmp_buffer_scoped::~tmp_buffer_scoped() { if(m_buf) m_buf->free(); }

void tmp_buffers::force_free() { tmp_buffer::force_free(); }
size_t tmp_buffers::get_total_size() { return tmp_buffer::get_total_size(); }
void tmp_buffers::enable_alloc_log(bool enable) { tmp_buffer::enable_alloc_log(enable); }

}
