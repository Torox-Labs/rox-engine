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
#include <string>
#include <vector>

namespace RoxResources
{

    class RoxFileResourcesProvider : public IRoxResourcesProvider
    {
    public:
        IRoxResourceData* access(const char* resource_name) override;
        bool has(const char* resource_name) override;

    public:
        bool setFolder(const char* folder_name, bool recursive = true, bool ignore_non_existent = false);

    public:
        int getResourcesCount() override;
        const char* getResourceName(int idx) override;

    public:
    	void lock() override;

    public:
        RoxFileResourcesProvider(const char* folder = "") { setFolder(folder); }

    private:
        void enumerateFolder(const char* folder_name);
        void updateNames();

    private:
        std::string m_path;
        bool m_recursive;
        bool m_update_names;
        std::vector<std::string> m_resource_names;
    };

}
