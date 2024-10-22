//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxTmpBuffers.h"
#include "RoxAlignAlloc.h"
#include "RoxMutex.h"
#include "RoxMemory.h"

#include <cstring>
#include <list>

namespace RoxMemory
{

class RoxTmpBuffers
{
private:
    void allocate(size_t size)
    {
        if(size>m_alloc_size)
        {
            if(m_allocate_log_enabled)
                RoxLogger::log()<<"tmp buf resized from "<<m_alloc_size<<" to "<<size<<", ";

            if(m_data)
                alignFree(m_data);

            m_data=(char *)alignAlloc(size,16);
            m_alloc_size=size;

            if(m_allocate_log_enabled)
                RoxLogger::log()<<getTotalSize()<<" in "<<m_buffers.size()<<" buffers total\n";
        }

        m_size=size;
        m_used=true;
    }

    bool isUsed() const { return m_used; }
    size_t getActualSize() const { return m_alloc_size; }

public:
    size_t getSize() const { return m_size; }

    void free()
    {
        RoxLockGuard guard(m_mutex);

        m_size=0;
        m_used=false;
    }

    void *getData(size_t offset)
    {
        if(offset>=m_size)
            return 0;

        return m_data+offset;
    }

    const void *getData(size_t offset) const
    {
        if(offset>=m_size)
            return 0;

        return m_data+offset;
    }

    bool copyTo(void *data,size_t size,size_t offset) const
    {
        if(size+offset>m_size)
            return false;

        memcpy(data,m_data+offset,size);
        return true;
    }

    bool copyFrom(const void *data,size_t size,size_t offset)
    {
        if(size+offset>m_size)
            return false;

        memcpy(m_data+offset,data,size);
        return true;
    }

    static RoxTmpBuffers *allocate_new(size_t size)
    {
        m_mutex.lock();

        RoxTmpBuffers* min_suit_buf=0;
        RoxTmpBuffers* max_buf=0;

        for(buffers_list::iterator it=m_buffers.begin();it!=m_buffers.end();++it)
        {
            RoxTmpBuffers &buffer = *it;
            if(buffer.isUsed())
                continue;

            if(buffer.getActualSize()>=size && (!min_suit_buf  || buffer.getActualSize()< min_suit_buf->getActualSize()))
                min_suit_buf=&buffer;

            if(max_buf)
            {
                if(buffer.getActualSize() > max_buf->getActualSize())
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

        m_buffers.push_back(RoxTmpBuffers());
        RoxTmpBuffers* result=&m_buffers.back();
        m_mutex.unlock();
        m_buffers.back().allocate(size);

        if(m_allocate_log_enabled) RoxLogger::log()<<"new tmp buf allocated ("<<m_buffers.size()<<" total)\n";

        return result;
    }

    static void forceFree()
    {
        RoxLockGuard guard(m_mutex);

        for(buffers_list::iterator it=m_buffers.begin();it!=m_buffers.end();++it)
        {
            RoxTmpBuffers &buffer = *it;
            if(buffer.isUsed())
                continue;

            if(!buffer.m_data)
                continue;

            alignFree(buffer.m_data);
            buffer.m_data=0;
            buffer.m_alloc_size=buffer.m_size=0;
        }
    }

    static size_t getTotalSize()
    {
        RoxLockGuard guard(m_mutex);

        size_t size=0;
        for(buffers_list::iterator it=m_buffers.begin();it!=m_buffers.end();++it)
        {
            RoxTmpBuffers &buffer = *it;
            size+=buffer.m_alloc_size;
        }

        return size;
    }

    static void enableAllocLog(bool enable) { m_allocate_log_enabled=enable; }

    RoxTmpBuffers(): m_used(false),m_data(0),m_size(0),m_alloc_size(0) {}

private:
    bool m_used;
    char *m_data;
    size_t m_size;
    size_t m_alloc_size;

private:
    typedef std::list<RoxTmpBuffers> buffers_list;
    static buffers_list m_buffers;
    static bool m_allocate_log_enabled;
    static RoxMutex m_mutex;
};

RoxTmpBuffers::buffers_list RoxTmpBuffers::m_buffers;
bool RoxTmpBuffers::m_allocate_log_enabled=false;
RoxMutex RoxTmpBuffers::m_mutex;

void *RoxTmpBufferRef::getData(size_t offset) const
{
    if(!m_buf)
        return 0;

    return m_buf->getData(offset);
}

size_t RoxTmpBufferRef::getSize() const
{
    if(!m_buf)
        return 0;

    return m_buf->getSize();
}

bool RoxTmpBufferRef::copyFrom(const void*data,size_t size,size_t offset)
{
    if(!m_buf)
        return false;

    return m_buf->copyFrom(data,size,offset);
}

bool RoxTmpBufferRef::copyTo(void*data,size_t size,size_t offset) const
{
    if(!m_buf)
        return false;

    return m_buf->copyTo(data,size,offset);
}

void RoxTmpBufferRef::allocate(size_t size)
{
    free();

    if(!size)
        return;

    m_buf=RoxTmpBuffers::allocate_new(size);
}

void RoxTmpBufferRef::free()
{
    if(!m_buf)
        return;

    m_buf->free();
    m_buf=0;
}

void *RoxTmpBufferScoped::getData(size_t offset) const { return m_buf?m_buf->getData(offset):0; }
size_t RoxTmpBufferScoped::getSize() const { return m_buf?m_buf->getSize():0;}
bool RoxTmpBufferScoped::copyFrom(const void*data,size_t size,size_t offset) { return m_buf?m_buf->copyFrom(data,size,offset):false; }
bool RoxTmpBufferScoped::copyTo(void*data,size_t size,size_t offset) const { return m_buf?m_buf->copyTo(data,size,offset):false; }

RoxTmpBufferScoped::RoxTmpBufferScoped(size_t size): m_buf(RoxTmpBuffers::allocate_new(size)) {}
void RoxTmpBufferScoped::free() { if(m_buf) m_buf->free(); m_buf=0; }
RoxTmpBufferScoped::~RoxTmpBufferScoped() { if(m_buf) m_buf->free(); }

void TmpBuffers::forceFree() { RoxTmpBuffers::forceFree(); }
size_t TmpBuffers::getTotalSize() { return RoxTmpBuffers::getTotalSize(); }
void TmpBuffers::enableAllocLog(bool enable) { RoxTmpBuffers::enableAllocLog(enable); }

}
