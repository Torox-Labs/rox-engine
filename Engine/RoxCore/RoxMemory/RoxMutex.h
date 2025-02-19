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
	class RoxMutex : public RoxNonCopyable
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

	class RoxLockGuard : public RoxNonCopyable
	{
	public:
		RoxLockGuard(RoxMutex& m): m_mutex(m) { m.lock(); }
		~RoxLockGuard() { m_mutex.unlock(); }

	private:
		RoxMutex& m_mutex;
	};

	class RoxLockGuardRead : public RoxNonCopyable
	{
	public:
		RoxLockGuardRead(RoxMutexRw& m): m_mutex(m) { m.lockRead(); }
		~RoxLockGuardRead() { m_mutex.unlockRead(); }

	private:
		RoxMutexRw& m_mutex;
	};

	class RoxLockGuardWrite : public RoxNonCopyable
	{
	public:
		RoxLockGuardWrite(RoxMutexRw& m): m_mutex(m) { m.lockWrite(); }
		~RoxLockGuardWrite() { m_mutex.unlockWrite(); }

	private:
		RoxMutexRw& m_mutex;
	};


}
