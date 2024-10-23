// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "RoxProxy.h"
#include "RoxRender/RoxRender.h"
#include "RoxScene.h"
#include "RoxSharedResources.h"
#include "shader.h"
#include "texture.h"
#include "RoxRender/RoxSkeleton.h"

namespace RoxScene
{

    class RoxMaterialInternal;
    typedef RoxMaterialInternal shared_material;

    class RoxMaterialInternal : public RoxSceneShared<shared_material>
    {
    public:
        static const char* default_pass;

    public:
        void set(const char* pass_name = default_pass) const;
        void unset() const;
        void skeletonChanged(const RoxRender::RoxSkeleton* skeleton) const;
        int getParamIdx(const char* name) const;
        int getTextureIdx(const char* semantics) const;
        bool release();

        RoxMaterialInternal() : m_last_set_pass_idx(-1), m_should_rebuild_passes_maps(false) {}

    private:
        friend class RoxMaterial;

        typedef RoxMath::Vector4 Param;
        typedef RoxProxy<Param> ParamProxy;

        class ParamArray
        {
        public:
            void setCount(int count) { m_params.resize(count); }
            int getCount() const { return static_cast<int>(m_params.size()); }
            void set(int idx, const Param& p) { if (idx >= 0 && idx < static_cast<int>(m_params.size())) m_params[idx] = p; }
            void set(int idx, float f0, float f1 = 0.0f, float f2 = 0.0f, float f3 = 0.0f);
            void set(int idx, const RoxMath::Vector3& v, float w = 0.0f);
            const Param& get(int idx) const;
            Param& get(int idx);
            const float* getBuf() const { return m_params.empty() ? nullptr : &m_params[0].x; }
            float* getBuf() { return m_params.empty() ? nullptr : &m_params[0].x; }

        private:
            std::vector<Param> m_params;
        };

        typedef RoxProxy<ParamArray> ParamArrayProxy;

        struct ParamHolder
        {
            std::string name;
            ParamProxy p;
            ParamArrayProxy a;

            void applyToShader(const shader& shader, int uniform_idx) const;
        };

        struct MaterialTexture
        {
            std::string semantics;
            texture_proxy RoxProxy;
        };

        class RoxPass
        {
        public:
            const char* getName() const { return m_name.c_str(); }
            RoxRender::State& getState() { return m_render_state; }
            const RoxRender::State& getState() const { return m_render_state; }
            const shader& getShader() const { return m_shader; }
            void setShader(const shader& shader);
            void setPassParam(const char* name, const Param& value); // Overrides material param

        public:
            RoxPass() : m_shader_changed(false) {}
            RoxPass(const RoxPass& p) { *this = p; }
            RoxPass& operator=(const RoxPass& p);

        private:
            friend class RoxMaterialInternal;
            friend class RoxMaterial;
            void updateMaps(const RoxMaterialInternal& m) const;
            void updatePassParams();

            std::string m_name;
            RoxRender::State m_render_state;
            shader m_shader;
            mutable bool m_shader_changed;
            mutable std::vector<int> m_uniforms_idxs_map;
            mutable std::vector<int> m_textures_slots_map;

            struct PassParam
            {
                std::string name;
                Param p;
                int uniform_idx;

                PassParam() : uniform_idx(-1) {}
            };

            std::vector<PassParam> m_pass_params;
        };

        int addPass(const char* pass_name);
        int getPassIdx(const char* pass_name) const;
        RoxPass& getPass(int idx);
        const RoxPass& getPass(int idx) const;
        void removePass(const char* pass_name);
        void updatePassesMaps() const;

    private:
        std::string m_name;
        std::vector<RoxPass> m_passes;
        mutable int m_last_set_pass_idx;
        mutable bool m_should_rebuild_passes_maps;
        mutable std::vector<ParamHolder> m_params;
        mutable std::vector<MaterialTexture> m_textures;
    };

    class RoxMaterial
    {
    public:
        static const char* default_pass;

    public:
        typedef RoxMaterialInternal::Param Param;
        typedef RoxMaterialInternal::ParamProxy ParamProxy;
        typedef RoxMaterialInternal::ParamArray ParamArray;
        typedef RoxMaterialInternal::ParamArrayProxy ParamArrayProxy;
        typedef RoxMaterialInternal::RoxPass Pass;

        bool load(const char* name);
        void unload() { m_internal.release(); }

        const char* getName() const { return internal().m_name.c_str(); }
        void setName(const char* name) { m_internal.m_name.assign(name ? name : ""); }

        int addPass(const char* pass_name) { return m_internal.addPass(pass_name); } // Returns existing if already present
        int getPassesCount() const { return static_cast<int>(m_internal.m_passes.size()); }
        int getPassIdx(const char* pass_name) const { return m_internal.getPassIdx(pass_name); }
        const char* getPassName(int idx) const { return m_internal.m_passes[idx].m_name.c_str(); }
        Pass& getPass(const char* pass_name) { return m_internal.getPass(m_internal.getPassIdx(pass_name)); }
        const Pass& getPass(const char* pass_name) const { return m_internal.getPass(m_internal.getPassIdx(pass_name)); }
        Pass& getPass(int idx) { return m_internal.getPass(idx); }
        const Pass& getPass(int idx) const { return m_internal.getPass(idx); }
        Pass& getDefaultPass() { return getPass(addPass(default_pass)); } // Adds the default pass if it is not present
        void removePass(const char* pass_name) { return m_internal.removePass(pass_name); }

        void setTexture(const char* semantics, const texture& tex);
        void setTexture(const char* semantics, const texture_proxy& RoxProxy);
        void setTexture(int idx, const texture& tex);
        void setTexture(int idx, const texture_proxy& RoxProxy);

        int getTexturesCount() const;
        const char* getTextureSemantics(int idx) const;
        int getTextureIdx(const char* semantics) const;
        const texture_proxy& getTexture(int idx) const;
        const texture_proxy& getTexture(const char* semantics) const;

        const char* getParamName(int idx) const;
        int getParamsCount() const;
        int getParamIdx(const char* name) const { return m_internal.getParamIdx(name); }

        void setParam(int idx, float f0, float f1 = 0.0f, float f2 = 0.0f, float f3 = 0.0f);
        void setParam(int idx, const RoxMath::Vector3& v, float w = 0.0f);
        void setParam(int idx, const Param& p);
        void setParam(int idx, const ParamProxy& p);
        void setParamArray(int idx, const ParamArray& a);
        void setParamArray(int idx, const ParamArrayProxy& p);

        void setParam(const char* name, float f0, float f1 = 0.0f, float f2 = 0.0f, float f3 = 0.0f);
        void setParam(const char* name, const RoxMath::Vector3& v, float w = 0.0f);
        void setParam(const char* name, const Param& p);
        void setParam(const char* name, const ParamProxy& p);
        void setParamArray(const char* name, const ParamArray& a);
        void setParamArray(const char* name, const ParamArrayProxy& p);

        const ParamProxy& getParam(int idx) const;
        const ParamArrayProxy& getParamArray(int idx) const;

        const ParamProxy& getParam(const char* name) const;
        const ParamArrayProxy& getParamArray(const char* name) const;

    public:
        RoxMaterial() {}
        RoxMaterial(const char* name) { *this = RoxMaterial(); load(name); }

    public:
        static void setResourcesPrefix(const char* prefix) { RoxMaterialInternal::setResourcesPrefix(prefix); }
        static void registerLoadFunction(RoxMaterialInternal::load_function function, bool clear_default = true) { RoxMaterialInternal::registerLoadFunction(function, clear_default); }

        // Debug purpose
    public:
        static void highlightMissingTextures(bool enable);
        static void globalTextureReplace(const char* semantics, const texture_proxy& tex);

    public:
        static bool loadText(shared_material& res, resource_data& data, const char* name);

        const RoxMaterialInternal& internal() const { return m_internal; }

    private:
        RoxMaterialInternal m_internal;
    };

}
