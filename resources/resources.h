//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "log/log.h"
#include "memory/mutex.h"
#include "memory/tmp_buffer.h"
#include <cstddef>

namespace nya_resources
{

class resource_data
{
public:
    virtual size_t get_size() { return 0; }

public:
    virtual bool read_all(void*data) { return false; }
    virtual bool read_chunk(void *data,size_t size,size_t offset=0) { return false; }

public:
    virtual void release() {}
};

class resources_provider
{
//thread-safe
public:
    virtual resource_data *access(const char *resource_name) { return 0; }
    virtual bool has(const char *resource_name) { return false; }

//requires lock
public:
    virtual int get_resources_count() { return 0; }
    virtual const char *get_resource_name(int idx) { return 0; }

//lock is for read-only operations
public:
    virtual void lock() { m_mutex.lock_read(); }
    virtual void unlock() { m_mutex.unlock_read(); }

protected:
    nya_memory::mutex_rw m_mutex;
};

void set_resources_path(const char *path); //sets default provider with path
void set_resources_provider(resources_provider *provider); //custom provider
resources_provider &get_resources_provider();

nya_memory::tmp_buffer_ref read_data(const char *name);

void set_log(nya_log::log_base *l);
nya_log::log_base &log();

bool check_extension(const char *name,const char *ext);

}
