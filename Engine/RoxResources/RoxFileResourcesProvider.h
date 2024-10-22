//nya-e(C) nyan.developer@gmail.com released under the MIT l(see LICENSE)

#pragma once

#include "RoxResources.h"
#include <string>
#include <vector>

namespace RoxResources
{

    class RoxFileResourcesProvider : public RoxResourcesProvider
    {
    public:
        RoxResourceData* access(const char* resourceName);
        bool has(const char* resourceName);

    public:
        bool setFolder(const char* folder, bool recursive = true, bool ignoreNonexistent = false);

    public:
        int getResourcesCount();
        const char* getResourceName(int idx);

    public:
        virtual void lock();

    public:
        RoxFileResourcesProvider(const char* folder = "") { setFolder(folder); }

    private:
        void enumerateFolder(const char* folderName);
        void updateNames();

    private:
        std::string m_path;
        bool m_recursive;
        bool m_update_names;
        std::vector<std::string> m_resource_names;
    };

}
