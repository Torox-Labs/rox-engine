// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

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
        virtual void lock() { m_mutex.lockRead(); }
        virtual void unlock() { m_mutex.unlockRead(); }

    protected:
        RoxMemory::RoxMutexRw m_mutex;
    };

    void setResourcesPath(const char* path); //sets default provider with path
    void setResourcesProvider(RoxResourcesProvider* provider); //custom provider
    RoxResourcesProvider& getResourcesProvider();

    RoxMemory::RoxTmpBufferRef readData(const char* name);

    void setLog(RoxLogger::RoxLoggerBase* l);
    RoxLogger::RoxLoggerBase& l();

    bool checkExtension(const char* name, const char* ext);

}
