// Updated By the ROX_ENGINE
// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
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

#include "RoxMutex.h"

namespace RoxMemory
{

void RoxMutex::lock()
{
#ifdef _MSC_VER
    m_mutex.lock();
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void RoxMutex::unlock()
{
#ifdef _MSC_VER
    m_mutex.unlock();
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

RoxMutex::RoxMutex()
{
#ifndef _MSC_VER
    pthread_mutex_init(&m_mutex,0);
#endif
}

RoxMutex::~RoxMutex()
{
#ifndef _MSC_VER
    pthread_mutex_destroy(&m_mutex);
#endif
}

void RoxMutexRw::lockRead()
{
#ifdef _MSC_VER
    m_mutex.lock();
    ++m_readers;
    m_mutex.unlock();
#elif !defined EMSCRIPTEN
    pthread_rwlock_rdlock(&m_mutex);
#endif
}

void RoxMutexRw::lockWrite()
{
#ifdef _MSC_VER
    m_mutex.lock();
    while(m_readers>0){}
#elif !defined EMSCRIPTEN
    pthread_rwlock_wrlock(&m_mutex);
#endif
}

void RoxMutexRw::unlockRead()
{
#ifdef _MSC_VER
    --m_readers;
#elif !defined EMSCRIPTEN
    pthread_rwlock_unlock(&m_mutex);
#endif
}

void RoxMutexRw::unlockWrite()
{
#ifdef _MSC_VER
    m_mutex.unlock();
#elif !defined EMSCRIPTEN
    pthread_rwlock_unlock(&m_mutex);
#endif
}

RoxMutexRw::RoxMutexRw()
{
#ifdef _MSC_VER
    m_readers=0;
#elif !defined EMSCRIPTEN
    pthread_rwlock_init(&m_mutex,0);
#endif
}

RoxMutexRw::~RoxMutexRw()
{
#if !defined _MSC_VER && !defined EMSCRIPTEN
    pthread_rwlock_destroy(&m_mutex);
#endif
}

}
