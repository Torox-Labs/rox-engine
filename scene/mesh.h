//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "shared_resources.h"
#include "material.h"
#include "animation.h"
#include "memory/memory_reader.h"
#include "render/vbo.h"
#include "render/skeleton.h"
#include "math/aabb.h"
#include "transform.h"

namespace nya_scene
{

struct shared_mesh
{
    nya_render::vbo vbo;

    nya_math::aabb aabb;

    struct aabb_bone_extend
    {
        int idx;
        float radius;
    };
    std::vector<aabb_bone_extend> aabb_bone_extends;

    struct group
    {
        std::string name;
        nya_math::aabb aabb;
        unsigned int material_idx;
        unsigned int offset;
        unsigned int count;
        nya_render::vbo::element_type elem_type;

        group(): material_idx(0),offset(0),count(0),elem_type(nya_render::vbo::triangles) {}
    };

    std::vector<group> groups;
    std::vector<material> materials;
    nya_render::skeleton skeleton;

    struct misc_info
    {
        std::string name, type;
        std::vector<std::pair<std::string, std::string> > string_params;
        std::vector<std::pair<std::string, nya_math::vec4> > vec4_params;
    };
    std::vector<misc_info> misc;

    bool release()
    {
        aabb=nya_math::aabb();
        vbo.release();
        groups.clear();
        materials.clear();
        skeleton=nya_render::skeleton();

        if(add_data)
        {
            delete add_data;
            add_data=0;
        }
        return true;
    }

    shared_mesh(): add_data(0) {}

    struct additional_data
    {
        virtual const char *type() { return 0; }
        virtual ~additional_data() {}
    };

    additional_data *add_data;
};

typedef proxy<animation> animation_proxy;

class mesh_internal: public scene_shared<shared_mesh>
{
    friend class mesh;

public:
    const transform &get_transform() const { return m_transform; }
    const nya_render::skeleton &get_skeleton() const { update_skeleton(); return m_skeleton; }
    int get_bones_count() const { return m_skeleton.get_bones_count(); }
    const char *get_bone_name(int idx) const { return m_skeleton.get_bone_name(idx); }
    int get_bone_idx(const char *name) const { return m_skeleton.get_bone_idx(name); }

private:
    mesh_internal(): m_recalc_aabb(true), m_has_aabb(false), need_update_skeleton(true) {}

    void draw_group(int idx, const char *pass_name) const;
    bool init_from_shared();

    int get_materials_count() const;
    const material &mat(int idx) const; //idx must be valid
    int get_mat_idx(int group_idx) const;

    struct applied_anim
    {
        int layer;
        float time;
        std::vector<int> bones_map;
        animation_proxy anim;
        unsigned int version;
        bool full_weight;
        bool lerp;

        applied_anim(): layer(0),time(0),version(0),lerp(false) {}
    };

    void anim_update_mapping(applied_anim &anim);
    void anim_set_time(applied_anim &anim,float time);
    bool is_anim_finished(int layer=0) const;

    void update(unsigned int dt);
    void update_skeleton() const;

    void update_aabb_transform() const;

private:
    enum bone_control_mode
    {
        bone_free,
        bone_additive,
        bone_override
    };

    struct bone_control
    {
        nya_math::vec3 pos;
        bone_control_mode pos_ctrl;
        nya_math::quat rot;
        bone_control_mode rot_ctrl;
        
        bone_control(): pos_ctrl(bone_free),rot_ctrl(bone_free) {}
    };

    transform m_transform;

    mutable bool need_update_skeleton;
    mutable nya_render::skeleton m_skeleton;
    std::vector<applied_anim> m_anims;
    typedef std::map<int,bone_control> bone_control_map;
    bone_control_map m_bone_controls;

    std::vector<int> m_replaced_materials_idx;
    std::vector<material> m_replaced_materials;

    mutable bool m_recalc_aabb;
    mutable nya_math::aabb m_aabb;
    bool m_has_aabb;

    struct group
    {
        mutable nya_math::aabb aabb;
        bool has_aabb;

        group(): has_aabb(false) {}
    };

    std::vector<group> m_groups;
};

class mesh
{
public:
    bool load(const char *name);
    void unload();

    void create(const shared_mesh &res);

    const char *get_name() const { return internal().get_name(); }

    void update(unsigned int dt);
    void draw(const char *pass_name=material::default_pass) const;
    void draw_group(int group_idx,const char *pass_name=material::default_pass) const;
    bool has_pass(const char *pass_name) const;

    const nya_math::aabb &get_aabb() const;

    // transform
    const nya_math::vec3 &get_pos() const { return internal().m_transform.get_pos(); }
    const nya_math::quat &get_rot() const { return internal().m_transform.get_rot(); }
    const nya_math::vec3 &get_scale() const { return internal().m_transform.get_scale(); }
    void set_pos(float x,float y,float z) { m_internal.m_transform.set_pos(x,y,z); m_internal.m_recalc_aabb=true; }
    void set_pos(const nya_math::vec3 &pos) { m_internal.m_transform.set_pos(pos); m_internal.m_recalc_aabb=true; }
    void set_rot(nya_math::angle_deg yaw,nya_math::angle_deg pitch,nya_math::angle_deg roll);
    void set_rot(const nya_math::quat &rot) { m_internal.m_transform.set_rot(rot); m_internal.m_recalc_aabb=true; }
    void set_scale(float sx,float sy,float sz) { m_internal.m_transform.set_scale(sx,sy,sz); m_internal.m_recalc_aabb=true; }
    void set_scale(const nya_math::vec3 &s) { m_internal.m_transform.set_scale(s.x,s.y,s.z); m_internal.m_recalc_aabb=true; }
    void set_scale(float s) { set_scale(s,s,s); }
    void set_transform(const transform &t) { m_internal.m_transform=t; m_internal.m_recalc_aabb=true; }

    // groups
    int get_groups_count() const;
    const char *get_group_name(int group_idx) const;
    const material &get_material(int group_idx) const;
    material &modify_material(int group_idx);
    bool set_material(int group_idx,const material &mat);

    // skeleton
    const nya_render::skeleton &get_skeleton() const { return internal().get_skeleton(); }
    int get_bones_count() const { return internal().get_bones_count(); }
    const char *get_bone_name(int idx) const { return internal().get_bone_name(idx); }
    int get_bone_idx(const char *name) const { return internal().get_bone_idx(name); }
    nya_math::vec3 get_bone_pos(int bone_idx,bool local=false,bool ignore_animations=false) const;
    nya_math::quat get_bone_rot(int bone_idx,bool local=false) const;
    nya_math::vec3 get_bone_pos(const char *name,bool local=false,bool ignore_animations=false) const
                   { return get_bone_pos(get_bone_idx(name),local,ignore_animations); }
    nya_math::quat get_bone_rot(const char *name,bool local=false) const { return get_bone_rot(get_bone_idx(name),local); }
    void set_bone_pos(int bone_idx,const nya_math::vec3 &pos,bool additive);
    void set_bone_rot(int bone_idx,const nya_math::quat &rot,bool additive);

    // animation
    void set_anim(const animation & anim,int layer=0,bool lerp=false) { set_anim(animation_proxy(anim),layer,lerp); }
    void set_anim(const animation_proxy & anim,int layer=0,bool lerp=false);
    void remove_anim(int layer=0) { set_anim(animation_proxy(),layer); }
    const animation_proxy & get_anim(int layer=0) const;
    unsigned int get_anim_time(int layer=0) const;
    bool is_anim_finished(int layer=0) const;
    void set_anim_time(unsigned int time,int layer=0);

public:
    mesh() {}
    mesh(const char *name) { *this=mesh(); load(name); }

public:
    static void set_resources_prefix(const char *prefix) { mesh_internal::set_resources_prefix(prefix); }
    static void register_load_function(mesh_internal::load_function function,bool clear_default=true) { mesh_internal::register_load_function(function,clear_default); }
public:
    static bool is_frustrum_cull_enabled();
    static void set_frustum_cull(bool enable);

public:
    static bool load_nms(shared_mesh &res,resource_data &data,const char* name);
    static bool load_nms_mesh_section(shared_mesh &res,const void *data,size_t size,int version);
    static bool load_nms_skeleton_section(shared_mesh &res,const void *data,size_t size,int version);
    static bool load_nms_material_section(shared_mesh &res,const void *data,size_t size,int version);
    static bool load_nms_general_section(shared_mesh &res,const void *data,size_t size,int version);

    const mesh_internal &internal() const { return m_internal; }

private:
    mesh_internal m_internal;
};

}
