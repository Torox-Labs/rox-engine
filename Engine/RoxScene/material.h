//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "render/render.h"
#include "scene.h"
#include "shader.h"
#include "texture.h"

namespace nya_scene
{

class material_internal;
typedef material_internal shared_material;

class material_internal: public scene_shared<shared_material>
{
public:
    static const char *default_pass;

public:
    void set(const char *pass_name=default_pass) const;
    void unset() const;
    void skeleton_changed(const nya_render::skeleton *skeleton) const;
    int get_param_idx(const char *name) const;
    int get_texture_idx(const char *semantics) const;
    bool release();

    material_internal(): m_last_set_pass_idx(-1),m_should_rebuild_passes_maps(false) {}

private:
    friend class material;

    typedef nya_math::vec4 param;
    typedef proxy<param> param_proxy;

    class param_array
    {
    public:
        void set_count(int count) { m_params.resize(count); }
        int get_count() const { return (int)m_params.size(); }
        void set(int idx,const param &p) { if(idx>=0 && idx<(int)m_params.size()) m_params[idx]=p; }
        void set(int idx,float f0,float f1=0.0f,float f2=0.0f,float f3=0.0f);
        void set(int idx,const nya_math::vec3 &v,float w=0.0f);
        const param &get(int idx) const;
        param &get(int idx);
        const float *get_buf() const { return m_params.empty()?0:&m_params[0].x; }
        float *get_buf() { return m_params.empty()?0:&m_params[0].x; }

    private:
        std::vector<param> m_params;
    };

    typedef proxy<param_array> param_array_proxy;

    struct param_holder
    {
        std::string name;
        param_proxy p;
        param_array_proxy a;

        void apply_to_shader(const shader &shader, int uniform_idx) const;
    };

    struct material_texture
    {
        std::string semantics;
        texture_proxy proxy;
    };

    class pass
    {
    public:
        const char *get_name() const {return m_name.c_str();}
        nya_render::state &get_state() {return m_render_state;}
        const nya_render::state &get_state() const {return m_render_state;}
        const shader &get_shader() const {return m_shader;}
        void set_shader(const shader &shader);
        void set_pass_param(const char *name,const param &value); //overrides material param

    public:
        pass(): m_shader_changed(false) { }
        pass(const pass &p) { *this=p; }
        pass &operator=(const pass &p);

    private:
        friend class material_internal;
        friend class material;
        void update_maps(const material_internal &m) const;
        void update_pass_params();

        std::string m_name;
        nya_render::state m_render_state;
        shader m_shader;
        mutable bool m_shader_changed;
        mutable std::vector<int> m_uniforms_idxs_map;
        mutable std::vector<int> m_textures_slots_map;

        struct pass_param
        {
            std::string name;
            param p;
            int uniform_idx;

            pass_param(): uniform_idx(-1) {}
        };

        std::vector<pass_param> m_pass_params;
    };

    int add_pass(const char *pass_name);
    int get_pass_idx(const char *pass_name) const;
    pass &get_pass(int idx);
    const pass &get_pass(int idx) const;
    void remove_pass(const char *pass_name);
    void update_passes_maps() const;

private:
    std::string m_name;
    std::vector<pass> m_passes;
    mutable int m_last_set_pass_idx;
    mutable bool m_should_rebuild_passes_maps;
    mutable std::vector<param_holder> m_params;
    mutable std::vector<material_texture> m_textures;
};

class material
{
public:
    static const char *default_pass;

public:
    typedef material_internal::param param;
    typedef material_internal::param_proxy param_proxy;
    typedef material_internal::param_array param_array;
    typedef material_internal::param_array_proxy param_array_proxy;
    typedef material_internal::pass pass;

    bool load(const char *name);
    void unload() { m_internal.release(); }

    const char *get_name() const { return internal().m_name.c_str(); }
    void set_name(const char*name) { m_internal.m_name.assign(name?name:""); }

    int add_pass(const char *pass_name) { return m_internal.add_pass(pass_name); } //returns existing if already present
    int get_passes_count() const {return (int)m_internal.m_passes.size();}
    int get_pass_idx(const char *pass_name) const { return m_internal.get_pass_idx(pass_name); }
    const char *get_pass_name(int idx) const { return m_internal.m_passes[idx].m_name.c_str(); }
    pass &get_pass(const char *pass_name) { return m_internal.get_pass(m_internal.get_pass_idx(pass_name)); }
    const pass &get_pass(const char *pass_name) const { return m_internal.get_pass(m_internal.get_pass_idx(pass_name)); }
    pass &get_pass(int idx) { return m_internal.get_pass(idx); }
    const pass &get_pass(int idx) const { return m_internal.get_pass(idx); }
    pass &get_default_pass() { return get_pass(add_pass(default_pass)); } //adds the default pass if it is not present
    void remove_pass(const char *pass_name) { return m_internal.remove_pass(pass_name); };

    void set_texture(const char *semantics,const texture &tex);
    void set_texture(const char *semantics,const texture_proxy &proxy);
    void set_texture(int idx,const texture &tex);
    void set_texture(int idx,const texture_proxy &proxy);

    int get_textures_count() const;
    const char *get_texture_semantics(int idx) const;
    int get_texture_idx(const char *semantics) const;
    const texture_proxy &get_texture(int idx) const;
    const texture_proxy &get_texture(const char *semantics) const;

    const char *get_param_name(int idx) const;
    int get_params_count() const;
    int get_param_idx(const char *name) const {return m_internal.get_param_idx(name);}

    void set_param(int idx,float f0,float f1=0.0f,float f2=0.0f,float f3=0.0f);
    void set_param(int idx,const nya_math::vec3 &v,float w=0.0f);
    void set_param(int idx,const param &p);
    void set_param(int idx,const param_proxy &p);
    void set_param_array(int idx,const param_array & a);
    void set_param_array(int idx,const param_array_proxy & p);

    void set_param(const char *name,float f0,float f1=0.0f,float f2=0.0f,float f3=0.0f);
    void set_param(const char *name,const nya_math::vec3 &v,float w=0.0f);
    void set_param(const char *name,const param &p);
    void set_param(const char *name,const param_proxy &p);
    void set_param_array(const char *name,const param_array & a);
    void set_param_array(const char *name,const param_array_proxy & p);

    const param_proxy &get_param(int idx) const;
    const param_array_proxy &get_param_array(int idx) const;

    const param_proxy &get_param(const char *name) const;
    const param_array_proxy &get_param_array(const char *name) const;

public:
    material() {}
    material(const char *name) { *this=material(); load(name); }

public:
    static void set_resources_prefix(const char *prefix) { material_internal::set_resources_prefix(prefix); }
    static void register_load_function(material_internal::load_function function,bool clear_default=true) { material_internal::register_load_function(function,clear_default); }

    //debug purpose
public:
    static void highlight_missing_textures(bool enable);
    static void global_texture_replace(const char *semantics,const texture_proxy &tex);

public:
    static bool load_text(shared_material &res,resource_data &data,const char* name);

    const material_internal &internal() const { return m_internal; }

private:
    material_internal m_internal;
};

}
