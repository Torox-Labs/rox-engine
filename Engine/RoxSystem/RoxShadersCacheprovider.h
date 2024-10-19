//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxRender/RoxShader.h"
#include <string>

namespace RoxRender{ class CompiledShader; }

namespace RoxSystem
{
    class RoxCompiledShadersProvider: public RoxRender::RoxCompiledShadersProvider
    {
    public:
        void setLoadPath(const char *path) { m_load_path.assign(path?path:""); }
        void setSavePath(const char *path) { m_save_path.assign(path?path:""); }

    public:
        static RoxCompiledShadersProvider &get()
        {
            static RoxCompiledShadersProvider csp;
            return csp;
        }

    public:
        bool get(const char *text,RoxRender::compiled_shader &shader);
        bool set(const char *text,const RoxRender::compiled_shader &shader);

    private:
        std::string crc(const char *text);

    private:
        std::string m_load_path;
        std::string m_save_path;
    };
}
