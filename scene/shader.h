//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "shared_resources.h"
#include "render/shader.h"
#include "render/skeleton.h"
#include "math/vector.h"
#include "memory/optional.h"
#include "scene/texture.h"

namespace nya_scene
{

struct shared_shader
{
    nya_render::shader shdr;

    std::vector<std::string> samplers;

    enum predefined_values
    {
        camera_pos=0,
        camera_rot,
        camera_dir,
        bones_pos,
        bones_pos_tr,
        bones_rot,
        bones_rot_tr,
        bones_pos_tex,
        bones_pos_tr_tex,
        bones_rot_tex,
        viewport,
        model_pos,
        model_rot,
        model_scale,

        predefines_count
    };

    enum transform_type
    {
        none,
        local,
        local_rot,
        local_rot_scale
    };

    struct predefined
    {
        predefined_values type;
        int location;
        transform_type transform;

        predefined(): location(-1), transform(none) {}
    };

    std::vector<predefined> predefines;

    struct uniform
    {
        std::string name;
        int location;
        transform_type transform;
        nya_math::vec4 default_value;

        uniform(): location(-1), transform(none) {}
    };

    std::vector<uniform> uniforms;

	shared_shader():last_skeleton_pos(0),last_skeleton_rot(0){}

    bool release()
    {
        shdr.release();
        predefines.clear();
        uniforms.clear();
        samplers.clear();
        if(texture_buffers.is_valid())
        {
            texture_buffers->skeleton_pos_texture.release();
            texture_buffers->skeleton_rot_texture.release();
            texture_buffers.free();
        }
        last_skeleton_pos=last_skeleton_rot=0;
        return true;
    }

    struct texture_buffers
    {
        unsigned int skeleton_pos_max_count;
        unsigned int skeleton_rot_max_count;
        nya_render::texture skeleton_pos_texture;
        nya_render::texture skeleton_rot_texture;
        const nya_render::skeleton *last_skeleton_pos_texture;
        const nya_render::skeleton *last_skeleton_rot_texture;

        texture_buffers():skeleton_pos_max_count(0),skeleton_rot_max_count(0),
                          last_skeleton_pos_texture(0),last_skeleton_rot_texture(0) {}
    };

    mutable nya_memory::optional<texture_buffers> texture_buffers;

    //cache
    const mutable nya_render::skeleton *last_skeleton_pos;
    const mutable nya_render::skeleton *last_skeleton_rot;
};

class shader_internal: public scene_shared<shared_shader>
{
public:
    void set() const;
    static void unset() { nya_render::shader::unbind(); }

    static void set_skeleton(const nya_render::skeleton *skeleton) { m_skeleton=skeleton; }
    void reset_skeleton() { if(!m_shared.is_valid()) return; m_shared->last_skeleton_pos=0; m_shared->last_skeleton_rot=0; }
    void skeleton_changed(const nya_render::skeleton *skeleton) const;

public:
    int get_texture_slot(const char *semantic) const;
    const char *get_texture_semantics(int slot) const;
    int get_texture_slots_count() const;

public:
    int get_uniforms_count() const;
    int get_uniform_idx(const char *name) const;
    const shared_shader::uniform &get_uniform(int idx) const;
    nya_render::shader::uniform_type get_uniform_type(int idx) const;
    unsigned int get_uniform_array_size(int idx) const;
    void set_uniform_value(int idx,float f0,float f1,float f2,float f3) const;
    void set_uniform4_array(int idx,const float *array,unsigned int count) const;

private:
    static const nya_render::skeleton *m_skeleton;
};

class shader
{
public:
    bool load(const char *name);
    void unload();

public:
    void create(const shared_shader &res);
    bool build(const char *code_text);

public:
    static void set_resources_prefix(const char *prefix) { shader_internal::set_resources_prefix(prefix); }
    static void register_load_function(shader_internal::load_function function,bool clear_default=true) { shader_internal::register_load_function(function,clear_default); }

public:
    const char *get_name() const { return m_internal.get_name(); }

public:
    shader() {}
    shader(const char *name) { *this=shader(); load(name); }
    ~shader() { unload(); }

public:
    static bool load_nya_shader(shared_shader &res,resource_data &data,const char* name);

public:
    const shader_internal &internal() const { return m_internal; }

private:
    shader_internal m_internal;
};

}
