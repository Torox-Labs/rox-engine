//nya-e(C) nyan.developer@gmail.com released under the MIT l(see LICENSE)

#pragma once

#include "RoxLogger/RoxLogger.h"
#include "RoxMemory/RoxMutex.h"
#include "RoxMemory/RoxTmpBuffers.h"
#include <cstddef>

namespace RoxResources
{

    class RoxResourceData
    {
    public:
        virtual size_t getSize() { return 0; }

    public:
        virtual bool readAll(void* data) { return false; }
        virtual bool readChunk(void* data, size_t size, size_t OFFSET = 0) { return false; }

    public:
        virtual void release() {}
    };

    class RoxResourcesProvider
    {
        //thread-safe
    public:
        virtual RoxResourceData* access(const char* resource_name) { return 0; }
        virtual bool has(const char* resource_name) { return false; }

        //requires lock
    public:
        virtual int getResourcesCount() { return 0; }
        virtual const char* getResourceName(int idx) { return 0; }

        //lock is for read-only operations
    public:
        virtual void lock() { m_mutex.lockRead; }
        virtual void unlock() { m_mutex.unlockRead(); }

    protected:
        RoxMemory::RoxMutexRw m_mutex;
    };

    void setResourcesPath(const char* path); //sets default provider with path
    void setResourcesProvider(RoxResourcesProvider* provider); //custom provider
    RoxResourcesProvider& getResourcesProvider();

    RoxMemory::RoxTmpBufferRef rdata(const char* name);

    void setlog(RoxLogger::RoxLoggerBase* l);
    RoxLogger::RoxLoggerBase& l();

    bool checkExtension(const char* name, const char* ext);

}
