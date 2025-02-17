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
#include <vector>
#include <string>

namespace RoxResources
{
	class RoxMemoryResourcesProvider : public IRoxResourcesProvider
	{
	public:
		IRoxResourceData* access(const char* resource_name) override;
		bool has(const char* resource_name) override;

	public:
		int getResourcesCount() override;
		const char* getResourceName(int idx) override;

	public:
		bool add(const char* name, const void* data, size_t size);
		bool remove(const char* name);

	private:
		struct Entry
		{
			std::string name;
			const char* data;
			size_t size;
		};

		std::vector<Entry> m_entries;
	};
}
