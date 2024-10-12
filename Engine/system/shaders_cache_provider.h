//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "render/shader.h"
#include <string>

namespace nya_render{ class compiled_shader; }

namespace nya_system
{
    class compiled_shaders_provider: public nya_render::compiled_shaders_provider
    {
    public:
        void set_load_path(const char *path) { m_load_path.assign(path?path:""); }
        void set_save_path(const char *path) { m_save_path.assign(path?path:""); }

    public:
        static compiled_shaders_provider &get()
        {
            static compiled_shaders_provider csp;
            return csp;
        }

    public:
        bool get(const char *text,nya_render::compiled_shader &shader);
        bool set(const char *text,const nya_render::compiled_shader &shader);

    private:
        std::string crc(const char *text);

    private:
        std::string m_load_path;
        std::string m_save_path;
    };
}
