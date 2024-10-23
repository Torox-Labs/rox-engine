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

#include "RoxResources.h"
#include <vector>
#include <string>

class AAssetManager;

namespace RoxResources
{

    class RoxApkResourcesProvider : public RoxResourcesProvider
    {
    public:
        RoxResourceData* access(const char* resource_name);
        bool has(const char* resource_name);

    public:
        bool setFolder(const char* folder);

    public:
        int get_resources_count();
        const char* getResourceName(int idx);

    public:
        virtual void lock();

    public:
        RoxApkResourcesProvider(const char* folder = "") { setFolder(folder); }

    public:
        static void setAssetManager(AAssetManager* mgr);

    private:
        void updateNames();

    private:
        std::vector<std::string> m_names;
        bool m_update_names;
        std::string m_folder;
    };

}