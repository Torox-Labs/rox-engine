//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxNonCopyable.h"
#include <cstddef>

//Note: many buffers are not supposed to be opened at the same time

namespace RoxMemory
{

class RoxTmpBuffers;

class RoxTmpBufferRef
{
    friend class RoxTmpBufferScoped;

public:
    bool copyFrom(const void*data,size_t size,size_t offset=0); //from data to buffer
    bool copyTo(void*data,size_t size,size_t offset=0) const; //from buffer to data

public:
    void *getData(size_t offset=0) const;
    size_t getSize() const;

public:
    void allocate(size_t size);
    void free();

public:
    RoxTmpBufferRef(): m_buf(0) {}
    RoxTmpBufferRef(size_t size)
    {
        m_buf=0;
        allocate(size);
    }

private:
    RoxTmpBuffers* m_buf;
};

class RoxTmpBufferScoped: public RoxNonCopyable
{
public:
    bool copyFrom(const void*data,size_t size,size_t offset=0); //from data to buffer
    bool copyTo(void*data,size_t size,size_t offset=0) const; //from buffer to data

public:
    void free();

public:
    void *getData(size_t offset=0) const;
    size_t getSize() const;

public:
    RoxTmpBufferScoped(size_t size);
    RoxTmpBufferScoped(const RoxTmpBufferRef &buf): m_buf(buf.m_buf) {}
    ~RoxTmpBufferScoped();

private:
    RoxTmpBuffers *m_buf;
};

namespace TmpBuffers
{
    void forceFree();
    size_t getTotalSize();
    void enableAllocLog(bool enable);
}

}
