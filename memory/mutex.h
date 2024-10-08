//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "non_copyable.h"

//ToDo: support mutex_rw in Emscripten

#ifdef _MSC_VER
    #include <mutex>
    #include <atomic>
#else
    #include <pthread.h>
#endif

namespace nya_memory
{

class mutex: public non_copyable
{
public:
    void lock();
    void unlock();

public:
    mutex();
    ~mutex();

private:
#ifdef _MSC_VER
    std::mutex m_mutex;
#else
    pthread_mutex_t m_mutex;
#endif
};

class mutex_rw: public non_copyable
{
public:
    void lock_read();
    void lock_write();
    void unlock_read();
    void unlock_write();

public:
    mutex_rw();
    ~mutex_rw();

private:
#ifdef _MSC_VER
    std::mutex m_mutex;
    std::atomic<int> m_readers;
#else
    pthread_rwlock_t m_mutex;
#endif
};

class lock_guard: public non_copyable
{
public:
    lock_guard(mutex &m): m_mutex(m) { m.lock(); }
    ~lock_guard() { m_mutex.unlock(); }

private:
    mutex &m_mutex;
};

class lock_guard_read: public non_copyable
{
public:
    lock_guard_read(mutex_rw &m): m_mutex(m) { m.lock_read(); }
    ~lock_guard_read() { m_mutex.unlock_read(); }

private:
    mutex_rw &m_mutex;
};

class lock_guard_write: public non_copyable
{
public:
    lock_guard_write(mutex_rw &m): m_mutex(m) { m.lock_write(); }
    ~lock_guard_write() { m_mutex.unlock_write(); }

private:
    mutex_rw &m_mutex;
};


}
