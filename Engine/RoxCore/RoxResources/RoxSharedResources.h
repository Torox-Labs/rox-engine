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

#include "RoxResources.h"
#include "RoxMemory/RoxPool.h"

#include <map>
#include <string>
#include <algorithm>

namespace RoxResources
{
	template <typename t_res, int block_count>
	class RoxSharedResources : public RoxMemory::RoxNonCopyable
	{
	private:
		virtual bool fillResource(const char* name, t_res& res) { return false; }
		virtual bool releaseResource(t_res& res) { return false; }

	private:
		class RoxSharedResourcesCreator
		{
			template <typename, int> friend class RoxSharedResources;
			struct ResHolder;

			class RoxSharedResourceRef
			{
				template <typename, int> friend class RoxSharedResources;

			public:
				bool isValid() const { return m_res != 0; }
				const t_res* constGet() const { return m_res; }
				const t_res* operator ->() const { return m_res; };

				const char* getName() const
				{
					if (!m_creator)
						return 0;

					return m_creator->getResName(*this); // back for this
				}

				int getRefCount() const
				{
					if (!m_creator)
						return 0;

					return m_creator->resGetRefCount(*this); // back for this
				}

				void free()
				{
					if (m_creator)
						m_creator->free(*this);

					m_res = 0;
					m_res_holder = 0;
					m_creator = 0;
				}

			public:
				RoxSharedResourceRef() : m_res(0), m_res_holder(0), m_creator(0)
				{
				}

				RoxSharedResourceRef(const RoxSharedResourceRef& ref)
				{
					m_res = ref.m_res;
					m_res_holder = ref.m_res_holder;
					m_creator = ref.m_creator;

					refCountInc();
				}

				RoxSharedResourceRef& operator=(const RoxSharedResourceRef& ref)
				{
					if (this == &ref)
						return *this;

					free();

					m_res = ref.m_res;
					m_res_holder = ref.m_res_holder;
					m_creator = ref.m_creator;

					refCountInc();

					return *this;
				}

				~RoxSharedResourceRef() { free(); }

			protected:
				RoxSharedResourceRef(t_res* res, ResHolder* holder, RoxSharedResourcesCreator* creator) :
					m_res(res), m_res_holder(holder), m_creator(creator)
				{
				}

			private:
				void refCountInc()
				{
					if (m_creator)
						m_creator->resRefCountInc(*this);
				}

			protected:
				t_res* m_res;

			private:
				ResHolder* m_res_holder;
				RoxSharedResourcesCreator* m_creator;
			};

			class RoxSharedResourceMutableRef : public RoxSharedResourceRef
			{
				template <typename, int> friend class RoxSharedResources;

			public:
				t_res* get() { return this->m_res; }
				t_res* operator ->() { return this->m_res; }

			public:
				RoxSharedResourceMutableRef()
				{
				}

			private:
				RoxSharedResourceMutableRef(t_res* res, ResHolder* holder, RoxSharedResourcesCreator* creator)
				{
					*(RoxSharedResourceRef*)this = RoxSharedResourceRef(res, holder, creator);
				}
			};

		public:
			RoxSharedResourceRef access(const char* name)
			{
				if (!name || !m_base)
					return RoxSharedResourceRef();

				std::string nameStr(name);
				if (m_force_lowercase)
					std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), std::tolower);

				std::pair<resources_map_iterator, bool> ir = m_res_map.insert(std::make_pair(nameStr, (ResHolder*)0));
				if (ir.second)
				{
					ResHolder* holder = m_res_pool.allocate();
					if (!holder)
						return RoxSharedResourceRef();

					ir.first->second = holder;
					if (!m_base->fillResource(name, holder->res))
					{
						m_res_map.erase(ir.first);
						return RoxSharedResourceRef();
					}
					holder->ref_count = 1;
					holder->map_it = ir.first;

					++m_ref_count;

					return RoxSharedResourceRef(&(holder->res), holder, this);
				}
				else
				{
					ResHolder* holder = ir.first->second;
					if (holder)
					{
						++holder->ref_count;

						return RoxSharedResourceRef(&(holder->res), holder, this);
					}
				}

				return RoxSharedResourceRef();
			}

			RoxSharedResourceMutableRef create()
			{
				ResHolder* holder = m_res_pool.allocate();
				if (!holder)
					return RoxSharedResourceMutableRef();

				holder->ref_count = 1;
				holder->map_it = m_res_map.end();

				++m_ref_count;

				return RoxSharedResourceMutableRef(&(holder->res), holder, this);
			}

			static RoxSharedResourceMutableRef modify(const RoxSharedResourceRef& ref)
			{
				if (!ref.isValid())
					return RoxSharedResourceMutableRef();

				resRefCountInc(ref);
				return RoxSharedResourceMutableRef(&(ref.m_res_holder->res), ref.m_res_holder, ref.m_creator);
			}

			static int resGetRefCount(const RoxSharedResourceRef& ref)
			{
				if (!ref.m_res_holder)
					return 0;

				return ref.m_res_holder->ref_count;
			}

			int reloadResources()
			{
				if (!m_base)
					return 0;

				int count = 0;

				for (resources_map_iterator it = m_res_map.begin();
				     it != m_res_map.end(); ++it)
				{
					if (!it->second || it->first.empty())
						continue;

					m_base->releaseResource(it->second->res);
					if (m_base->fillResource(it->first.c_str(), it->second->res))
						++count;
				}

				return count;
			}

			bool reloadResource(const char* name)
			{
				if (!name || !m_base)
					return false;

				std::string nameStr(name);
				if (m_force_lowercase)
					std::transform(nameStr.begin(), nameStr.end(), nameStr.begin(), ::tolower);

				resources_map_iterator it = m_res_map.find(nameStr);
				if (it == m_res_map.end() || !it->second)
					return false;

				m_base->reload_resource(it->second->res);
				return m_base->fillResource(it->first.c_str(), it->second->res);
			}

			const char* getResName(const RoxSharedResourceRef& ref)
			{
				if (!ref.m_res_holder)
					return 0;

				if (ref.m_creator != this)
					return 0;

				if (ref.m_res_holder->map_it == m_res_map.end())
					return 0;

				return ref.m_res_holder->map_it->first.c_str();
			}

			void free(RoxSharedResourceRef& ref)
			{
				if (!ref.m_res_holder)
					return;

				if (ref.m_creator != this)
					return;

				--ref.m_res_holder->ref_count;

				if (ref.m_res_holder->ref_count > 0)
					return;

				ref.m_res_holder->ref_count = 0;

				if (!m_should_unload_unused)
					return;

				if (m_ref_count > 0)
					--m_ref_count;
				else
					RoxLogger::log() << "resource system failure\n";

				if (ref.m_res && m_base)
					m_base->releaseResource(*ref.m_res);

				if (ref.m_res_holder->map_it != m_res_map.end())
				{
					if (!m_base)
						RoxLogger::log() << "warning: unreleased resource " << ref.m_res_holder->map_it->first.c_str()
							<< "\n";

					m_res_map.erase(ref.m_res_holder->map_it);
				}

				m_res_pool.free(ref.m_res_holder);

				if (!m_ref_count)
				{
					if (!m_base)
						delete this;
					else
						RoxLogger::log() << "resource system failure\n";
				}
			}

			static void resRefCountInc(const RoxSharedResourceRef& ref)
			{
				if (!ref.m_res_holder)
					return;

				++ref.m_res_holder->ref_count;
			}

			void shouldUnloadUnused(bool unload)
			{
				if (unload && unload != m_should_unload_unused)
					freeUnused();

				m_should_unload_unused = unload;
			}

			void freeUnused()
			{
				resources_map_iterator it = m_res_map.begin();
				while (it != m_res_map.end())
				{
					if (it->second)
					{
						if (it->second->ref_count > 0)
						{
							++it;
							continue;
						}

						if (m_base)
							m_base->releaseResource(it->second->res);

						m_res_pool.free(it->second);
					}

					resources_map_iterator er = it;
					++it;

					m_res_map.erase(er);
				}
			}

			void freeAll()
			{
				resources_map_iterator it;
				for (it = m_res_map.begin(); it != m_res_map.end(); ++it)
				{
					if (!it->second)
						continue;

					if (m_base)
						m_base->releaseResource(it->second->res);

					it->second->map_it = m_res_map.end();
				}

				m_res_map.clear();
				m_res_pool.clear();
			}

			void baseReleased()
			{
				if (!m_base)
					return;

				m_base = 0;

				if (m_ref_count > 0)
					--m_ref_count;
			}

			bool hasRefs() { return m_ref_count > 0; }

		public:
			RoxSharedResourcesCreator(RoxSharedResources* base) : m_base(base), m_should_unload_unused(true),
			                                                      m_force_lowercase(true), m_ref_count(1)
			{
			}

		private:
			typedef std::map<std::string, ResHolder*> resources_map;
			typedef typename resources_map::iterator resources_map_iterator;

		private:
			struct ResHolder
			{
				t_res res;
				int ref_count;
				resources_map_iterator map_it;

				ResHolder() : ref_count(0)
				{
				}
			};

			resources_map m_res_map;
			RoxMemory::pool<ResHolder, block_count> m_res_pool;

		private:
			RoxSharedResources* m_base;
			bool m_should_unload_unused;
			bool m_force_lowercase;
			size_t m_ref_count;
		}; // End of RoxSharedResourcesCreator

	public:
		typedef typename RoxSharedResourcesCreator::RoxSharedResourceRef RoxSharedResourceRef;
		typedef typename RoxSharedResourcesCreator::RoxSharedResourceMutableRef RoxSharedResourceMutableRef;

	public:
		RoxSharedResourceRef access(const char* name) { return m_creator->access(name); }
		RoxSharedResourceMutableRef create() { return m_creator->create(); }

		static RoxSharedResourceMutableRef modify(RoxSharedResourceRef& res)
		{
			return RoxSharedResourcesCreator::modify(res);
		}

		RoxSharedResources() { m_creator = new RoxSharedResourcesCreator(this); }

	public:
		//void free_all() { m_creator->free_all(); }
		void freeUnused() { m_creator->free_unused(); }
		void forceLowercase(bool force) { m_creator->m_force_lowercase = force; }
		void shouldUnloadUnused(bool unload) { m_creator->shouldUnloadUnused(unload); }
		bool reloadResource(const char* name) { return m_creator->reload_resource(name); }
		int reloadResources() { return m_creator->reload_resources(); }

	public:
		RoxSharedResourceRef getFirstResource()
		{
			if (m_creator->m_res_map.empty())
				return RoxSharedResourceRef();

			struct RoxSharedResourcesCreator::ResHolder* holder = m_creator->m_res_map.begin()->second;
			if (!holder)
				return RoxSharedResourceRef();

			++holder->ref_count;
			return RoxSharedResourceRef(&(holder->res), holder, m_creator);
		}

		RoxSharedResourceRef getNextResource(RoxSharedResourceRef& curr)
		{
			if (curr.m_creator != m_creator || !curr.m_res_holder)
				return RoxSharedResourceRef();

			typename RoxSharedResourcesCreator::resources_map_iterator it = curr.m_res_holder->map_it;
			if (++it == m_creator->m_res_map.end())
				return RoxSharedResourceRef();

			struct RoxSharedResourcesCreator::ResHolder* holder = it->second;
			if (!holder)
				return RoxSharedResourceRef();

			++holder->ref_count;
			return RoxSharedResourceRef(&(holder->res), holder, m_creator);
		}

	public:
		virtual ~RoxSharedResources()
		{
			m_creator->baseReleased();
			if (!m_creator->hasRefs())
				delete m_creator;
		}

	private:
		class RoxSharedResourcesCreator* m_creator;
	};
}
