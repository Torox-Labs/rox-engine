//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

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