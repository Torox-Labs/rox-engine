// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxResources/RoxSharedResources.h"
#include "RoxMemory/RoxTmpBuffers.h"
#include "RoxRender/RoxAnimation.h"
#include <string>
#include <vector>
#include <map>

namespace RoxScene { class MeshInternal; }

namespace RoxScene
{

    using ResourceData = RoxMemory::RoxTmpBufferRef;

    template<typename T>
    class RoxSceneShared
    {
        friend class MeshInternal;

    public:
        bool load(const char* name)
        {
            if (!name || !name[0])
            {
                unload();
                return false;
            }

            const std::string finalName = getResourcesPrefixStr() + name;
            if (mShared.is_valid())
            {
                const char* resName = mShared.get_name();
                if (resName && finalName == resName)
                    return true;
            }

            unload();

            mShared = getSharedResources().access(finalName.c_str());

            return mShared.is_valid();
        }

        void create(const T& res)
        {
            typename SharedResources::SharedResourceMutableRef ref = getSharedResources().create();
            if (!ref.is_valid())
            {
                unload();
                return;
            }

            *ref.get() = res;
            mShared = ref;
        }

        void unload()
        {
            if (mShared.is_valid())
                mShared.free();
        }

        const char* getName() const { return mShared.get_name(); }

    public:
        static void setResourcesPrefix(const char* prefix) { getResourcesPrefixStr().assign(prefix ? prefix : ""); }
        static const char* getResourcesPrefix() { return getResourcesPrefixStr().c_str(); }

    public:
        static int reloadAllResources() { return getSharedResources().reload_resources(); }

        static bool reloadResource(const char* name)
        {
            if (!name)
                return false;

            if (getResourcesPrefixStr().empty())
                return getSharedResources().reload_resource(name);

            return getSharedResources().reload_resource((getResourcesPrefixStr() + name).c_str());
        }

    public:
        using LoadFunction = bool(*)(T& sh, ResourceData& data, const char* name);

        static void registerLoadFunction(LoadFunction function, bool clearDefault)
        {
            if (!function)
                return;

            if (clearDefault)
            {
                getLoadFunctions().clearDefault = true;
                for (int i = 0; i < static_cast<int>(getLoadFunctions().f.size()); )
                {
                    if (getLoadFunctions().f[i].second)
                        getLoadFunctions().f.erase(getLoadFunctions().f.begin() + i);
                    else
                        ++i;
                }
            }

            getLoadFunctions().f.emplace_back(function, false);
        }

        static void defaultLoadFunction(LoadFunction function)
        {
            if (!function)
                return;

            if (getLoadFunctions().clearDefault)
                return;

            getLoadFunctions().f.emplace_back(function, true);
        }

    public:
        virtual ~RSceneShared() {}

    protected:
        using SharedResources = RoxResources::RoxSharedResources<T, 8>;
        using SharedResourceRef = typename SharedResources::RoxSharedResourceRef;

        class SharedResourcesManager : public SharedResources
        {
        public:
            bool fillResource(const char* name, T& res)
            {
                if (!name)
                {
                    RoxLogger::log() << "Unable to load scene resource: invalid name\n";
                    return false;
                }

                RoxResources::RoxResourceData* fileData = RoxResources::getResourcesProvider().access(name);
                if (!fileData)
                {
                    RoxLogger::log() << "Unable to load scene resource: unable to access resource " << name << "\n";
                    return false;
                }

                const std::size_t dataSize = fileData->getSize();
                RoxMemory::RoxTmpBufferRef resData(dataSize);
                fileData->readAll(resData.getData());
                fileData->release();

                for (size_t i = 0; i < RoxSceneShared::getLoadFunctions().f.size(); ++i)
                {
                    if (RoxSceneShared::getLoadFunctions().f[i].first(res, resData, name))
                    {
                        resData.free();
                        return true;
                    }

                    // Optionally reset or release resource if needed
                }

                resData.free();
                RoxLogger::log() << "Unable to load scene resource: unknown format or invalid data in " << name << "\n";
                return false;
            }

            bool releaseResource(T& res)
            {
                return res.release();
            }
        };

    public:
        static SharedResourcesManager& getSharedResources()
        {
            static SharedResourcesManager manager;
            return manager;
        }

    public:
        const SharedResourceRef& getSharedData() const { return mShared; }

    private:
        struct LoadFunctions
        {
            std::vector<std::pair<LoadFunction, bool>> f;
            bool clearDefault;

            void add(LoadFunction function, bool isDefault)
            {
                if (!function)
                    return;

                for (const auto& pair : f)
                    if (pair.first == function)
                        return;

                f.emplace_back(function, isDefault);
            }

            LoadFunctions() : clearDefault(false) {}
        };

        static LoadFunctions& getLoadFunctions()
        {
            static LoadFunctions functions;
            return functions;
        }

    private:
        static std::string& getResourcesPrefixStr()
        {
            static std::string prefix;
            return prefix;
        }

    protected:
        SharedResourceRef m_shared;
    };

} // namespace RoxScene
