//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "non_copyable.h"
#include <cstddef>

//Note: many buffers are not supposed to be opened at the same time

namespace nya_memory
{

class tmp_buffer;

class tmp_buffer_ref
{
    friend class tmp_buffer_scoped;

public:
    bool copy_from(const void*data,size_t size,size_t offset=0); //from data to buffer
    bool copy_to(void*data,size_t size,size_t offset=0) const; //from buffer to data

public:
    void *get_data(size_t offset=0) const;
    size_t get_size() const;

public:
    void allocate(size_t size);
    void free();

public:
    tmp_buffer_ref(): m_buf(0) {}
    tmp_buffer_ref(size_t size)
    {
        m_buf=0;
        allocate(size);
    }

private:
    tmp_buffer* m_buf;
};

class tmp_buffer_scoped: public non_copyable
{
public:
    bool copy_from(const void*data,size_t size,size_t offset=0); //from data to buffer
    bool copy_to(void*data,size_t size,size_t offset=0) const; //from buffer to data

public:
    void free();

public:
    void *get_data(size_t offset=0) const;
    size_t get_size() const;

public:
    tmp_buffer_scoped(size_t size);
    tmp_buffer_scoped(const tmp_buffer_ref &buf): m_buf(buf.m_buf) {}
    ~tmp_buffer_scoped();

private:
    tmp_buffer *m_buf;
};

namespace tmp_buffers
{
    void force_free();
    size_t get_total_size();
    void enable_alloc_log(bool enable);
}

}
