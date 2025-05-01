//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxResources/RoxSharedResources.h"
#include "RoxMemory/RoxTmpBuffers.h"

namespace RoxScene
{

    typedef RoxMemory::RoxTmpBufferRef resource_data;

    template<typename t>
    class scene_shared
    {
    public:
        bool load(const char* name)
        {
            if (!name || !name[0])
            {
                unload();
                return false;
            }

            const std::string final_name = get_resources_prefix_str() + name;
            if (m_shared.isValid())
            {
                const char* res_name = m_shared.getName();
                if (res_name && final_name == res_name)
                    return true;
            }

            unload();

            m_shared = get_shared_resources().access(final_name.c_str());

            return m_shared.isValid();
        }

        void create(const t& res)
        {
            typename shared_resources::RoxSharedResourceMutableRef ref = get_shared_resources().create();
            if (!ref.isValid())
            {
                unload();
                return;
            }

            *ref.get() = res;
            m_shared = ref;
        }

        void unload()
        {
            if (m_shared.isValid())
                m_shared.free();
        }

        const char* getName() const { return m_shared.getName(); }

    public:
        static void set_resources_prefix(const char* prefix) { get_resources_prefix_str().assign(prefix ? prefix : ""); }
        static const char* get_resources_prefix() { return get_resources_prefix_str().c_str(); }

    public:
        static int reload_all_resources() { return get_shared_resources().reload_resources(); }

        static bool reload_resource(const char* name)
        {
            if (!name)
                return false;

            if (get_resources_prefix_str().empty())
                return get_shared_resources().reload_resource(name);

            return get_shared_resources().reload_resource((get_resources_prefix_str() + name).c_str());
        }

    public:
        typedef bool (*load_function)(t& sh, resource_data& data, const char* name);

        static void register_load_function(load_function function, bool clear_default)
        {
            if (!function)
                return;

            if (clear_default)
            {
                get_load_functions().clear_default = true;
                for (int i = 0; i < (int)get_load_functions().f.size();)
                {
                    if (get_load_functions().f[i].second)
                        get_load_functions().f.erase(get_load_functions().f.begin() + i);
                    else
                        ++i;
                }
            }

            get_load_functions().add(function, false);
        }

        static void default_load_function(load_function function)
        {
            if (!function)
                return;

            if (get_load_functions().clear_default)
                return;

            get_load_functions().add(function, true);
        }

    public:
        virtual ~scene_shared<t>() {}

    protected:
        typedef RoxResources::RoxSharedResources<t, 8> shared_resources;
        typedef typename shared_resources::RoxSharedResourceRef shared_resource_ref;

        class shared_resources_manager : public shared_resources
        {
            bool fillResource(const char* name, t& res)
            {
                if (!name)
                {
                    RoxResources::log() << "unable to load scene resource: invalid name\n";
                    return false;
                }

                RoxResources::IRoxResourceData* file_data = RoxResources::getResourcesProvider().access(name);
                if (!file_data)
                {
                    RoxResources::log() << "unable to load scene resource: unable to access resource " << name << "\n";
                    return false;
                }

                const size_t data_size = file_data->getSize();
                RoxMemory::RoxTmpBufferRef res_data(data_size);
                file_data->readAll(res_data.getData());
                file_data->release();

                for (size_t i = 0; i < scene_shared::get_load_functions().f.size(); ++i)
                {
                    if (scene_shared::get_load_functions().f[i].first(res, res_data, name))
                    {
                        res_data.free();
                        return true;
                    }

                    //res.free(),res=t();
                }

                res_data.free();
                //res.free(),res=t();
                RoxResources::log() << "unable to load scene resource: unknown format or invalid data in " << name << "\n";
                return false;
            }

            bool release_resource(t& res)
            {
                return res.release();
            }
        };

    public:
        static shared_resources_manager& get_shared_resources()
        {
            static shared_resources_manager manager;
            return manager;
        }

    public:
        const shared_resource_ref& get_shared_data() const { return m_shared; }

    private:
        struct load_functions
        {
            std::vector<std::pair<load_function, bool> > f;
            bool clear_default;

            void add(load_function function, bool is_default)
            {
                if (!function)
                    return;

                for (size_t i = 0; i < f.size(); ++i)
                    if (f[i].first == function)
                        return;

                f.resize(f.size() + 1);
                f.back().first = function;
                f.back().second = is_default;
            }

            load_functions() : clear_default(false) {}
        };

        static load_functions& get_load_functions()
        {
            static load_functions functions;
            return functions;
        }

    private:
        static std::string& get_resources_prefix_str()
        {
            static std::string prefix;
            return prefix;
        }

    protected:
        shared_resource_ref m_shared;
    };

}