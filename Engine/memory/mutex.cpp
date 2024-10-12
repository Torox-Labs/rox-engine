//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "mutex.h"

namespace nya_memory
{

void mutex::lock()
{
#ifdef _MSC_VER
    m_mutex.lock();
#else
    pthread_mutex_lock(&m_mutex);
#endif
}

void mutex::unlock()
{
#ifdef _MSC_VER
    m_mutex.unlock();
#else
    pthread_mutex_unlock(&m_mutex);
#endif
}

mutex::mutex()
{
#ifndef _MSC_VER
    pthread_mutex_init(&m_mutex,0);
#endif
}

mutex::~mutex()
{
#ifndef _MSC_VER
    pthread_mutex_destroy(&m_mutex);
#endif
}

void mutex_rw::lock_read()
{
#ifdef _MSC_VER
    m_mutex.lock();
    ++m_readers;
    m_mutex.unlock();
#elif !defined EMSCRIPTEN
    pthread_rwlock_rdlock(&m_mutex);
#endif
}

void mutex_rw::lock_write()
{
#ifdef _MSC_VER
    m_mutex.lock();
    while(m_readers>0){}
#elif !defined EMSCRIPTEN
    pthread_rwlock_wrlock(&m_mutex);
#endif
}

void mutex_rw::unlock_read()
{
#ifdef _MSC_VER
    --m_readers;
#elif !defined EMSCRIPTEN
    pthread_rwlock_unlock(&m_mutex);
#endif
}

void mutex_rw::unlock_write()
{
#ifdef _MSC_VER
    m_mutex.unlock();
#elif !defined EMSCRIPTEN
    pthread_rwlock_unlock(&m_mutex);
#endif
}

mutex_rw::mutex_rw()
{
#ifdef _MSC_VER
    m_readers=0;
#elif !defined EMSCRIPTEN
    pthread_rwlock_init(&m_mutex,0);
#endif
}

mutex_rw::~mutex_rw()
{
#if !defined _MSC_VER && !defined EMSCRIPTEN
    pthread_rwlock_destroy(&m_mutex);
#endif
}

}
