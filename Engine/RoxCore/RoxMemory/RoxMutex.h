//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxNonCopyable.h"

//TODO: support RoxMutexRw in Emscripten

#ifdef _MSC_VER
    #include <mutex>
    #include <atomic>
#else
    #include <pthread.h>
#endif

namespace RoxMemory
{

class RoxMutex: public RoxNonCopyable
{
public:
    void lock();
    void unlock();

public:
    RoxMutex();
    ~RoxMutex();

private:
#ifdef _MSC_VER
    std::mutex m_mutex;
#else
    pthread_mutex_t m_mutex;
#endif
};

class RoxMutexRw : public RoxNonCopyable
{
public:
    void lockRead();
    void lockWrite();
    void unlockRead();
    void unlockWrite();

public:
    RoxMutexRw();
    ~RoxMutexRw();

private:
#ifdef _MSC_VER
    std::mutex m_mutex;
    std::atomic<int> m_readers;
#else
    pthread_rwlock_t m_mutex;
#endif
};

class RoxLockGuard: public RoxNonCopyable
{
public:
    RoxLockGuard(RoxMutex &m): m_mutex(m) { m.lock(); }
    ~RoxLockGuard() { m_mutex.unlock(); }

private:
    RoxMutex &m_mutex;
};

class RoxLockGuardRead: public RoxNonCopyable
{
public:
    RoxLockGuardRead(RoxMutexRw &m): m_mutex(m) { m.lockRead(); }
    ~RoxLockGuardRead() { m_mutex.unlockRead(); }

private:
    RoxMutexRw &m_mutex;
};

class RoxLockGuardWrite: public RoxNonCopyable
{
public:
    RoxLockGuardWrite(RoxMutexRw &m): m_mutex(m) { m.unlockWrite(); }
    ~RoxLockGuardWrite() { m_mutex.unlockWrite(); }

private:
    RoxMutexRw &m_mutex;
};


}
