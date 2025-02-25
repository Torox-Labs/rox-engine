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

#include <vector>
#include <list>

namespace RoxRender
{
	template <typename t>
	class RoxRenderObjects
	{
	public:
		t& get(int idx) { return m_objects[idx].data; }

		void remove(int idx)
		{
			if (!m_objects[idx].free)
			{
				m_objects[idx].free = true;
				m_objects[idx].data.release();
				m_free.push_back(idx);
			}
		}

		int add()
		{
			if (!m_free.empty())
			{
				const int idx = m_free.back();
				m_free.pop_back();
				m_objects[idx].free = false;
				//m_objects[idx].data=t(); //not counting on release

				return idx;
			}

			const int idx = (int)m_objects.size();
			m_objects.resize(m_objects.size() + 1);
			m_objects[idx].free = false;

			return idx;
		}

		int getCount() { return int(m_objects.size()) - int(m_free.size()); }

		template <typename ta>
		int applyToAll(ta& applier)
		{
			int count = 0;
			for (int i = 0; i < (int)m_objects.size(); ++i)
			{
				if (m_objects[i].free)
					continue;

				applier.apply(m_objects[i].data), ++count;
			}
			return count;
		}

		int releaseAll()
		{
			int count = 0;
			for (int i = 0; i < (int)m_objects.size(); ++i)
			{
				if (m_objects[i].free)
					continue;

				m_objects[i].data.release(), ++count;
			}

			return count;
		}

		int invalidateAll()
		{
			int count = 0;
			for (int i = 0; i < (int)m_objects.size(); ++i)
			{
				if (m_objects[i].free)
					continue;

				m_objects[i].data = t(), ++count;
			}
			return count;
		}

	private:
		struct Object
		{
			bool free;
			t data;
		};

		std::vector<Object> m_objects;
		std::list<int> m_free;
	};

}
