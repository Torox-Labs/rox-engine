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

#include "RoxRender/RoxShader.h"
#include <string>

namespace RoxRender{ class CompiledShader; }

namespace RoxSystem
{
    class RoxShaderCacheProvider: public RoxRender::IRoxCompiledShadersProvider
    {
    public:
        void setLoadPath(const char *path) { m_load_path.assign(path?path:""); }
        void setSavePath(const char *path) { m_save_path.assign(path?path:""); }

    public:
        static RoxShaderCacheProvider&get()
        {
            static RoxShaderCacheProvider csp;
            return csp;
        }

    public:
        bool get(const char *text,RoxRender::RoxCompiledShader &shader) override;
        bool set(const char *text,const RoxRender::RoxCompiledShader &shader) override;

    private:
        std::string crc(const char *text);

    private:
        std::string m_load_path;
        std::string m_save_path;
    };
}
