//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "memory/tag_list.h"
#include "mesh.h"
#include "tags.h"

namespace nya_scene
{

struct shared_location
{
    struct location_mesh
    {
        std::string name;
        tags tg;
        transform tr;
    };

    std::vector<location_mesh> meshes;

    std::vector<std::pair<std::string,nya_math::vec4> > material_params;
    std::vector<std::pair<std::string,nya_math::vec4> > vec_params;
    std::vector<std::pair<std::string,std::string> > string_params;

    bool release() { *this=shared_location(); return true; }
};

class location: public scene_shared<shared_location>
{
public:
    bool load(const char *name);
    void unload();

public:
    int add_mesh(const tags &tg,const transform &tr) { return add_mesh(0,tg,tr); }
    int add_mesh(const char *mesh_name,const tags &tg,const transform &tr);
    void remove_mesh(int idx) { m_meshes.remove(idx); }

    int get_meshes_count() const { return m_meshes.get_count(); }
    const mesh &get_mesh(int idx) const { return m_meshes.get(idx).m; }
    mesh &modify_mesh(int idx,bool need_apply=true);

    int get_meshes_count(const char *tag) const { return m_meshes.get_count(tag); }
    const mesh &get_mesh(const char *tag,int idx) const { return m_meshes.get(tag,idx).m; }
    mesh &modify_mesh(const char *tag,int idx,bool need_apply=true);

    void set_mesh_visible(int idx,bool visible) { m_meshes.get(idx).visible=visible; }
    bool is_mesh_visible(int idx) const { return m_meshes.get(idx).visible; }

public:
    int get_material_params_count() const { return (int)m_material_params.size(); }
    const char *get_material_param_name(int idx) const;
    const material::param_proxy &get_material_param(int idx) const;
    void set_material_param(int idx,const material::param &p);

public:
    void update(int dt);
    void draw(const char *pass=material::default_pass,const tags &t=0) const;

public:
    location(): m_need_apply(false) {}
    location(const char *name) { load(name); }

public:
    static bool load_text(shared_location &res,resource_data &data,const char* name);

private:
    struct location_mesh
    {
        mesh m;
        bool visible;
        bool need_apply;

        location_mesh(): visible(false), need_apply(false) {}
    };

    nya_memory::tag_list<location_mesh> m_meshes;
    mutable std::vector<bool> m_draw_cache;
    std::vector<std::pair<std::string,material::param_proxy> > m_material_params;
    bool m_need_apply;
};

}
