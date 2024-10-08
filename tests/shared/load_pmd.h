//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "math/aabb.h"
#include <vector>
#include <string>

namespace nya_memory { class tmp_buffer_ref; }
namespace nya_scene { class mesh; struct shared_mesh; class texture; typedef nya_memory::tmp_buffer_ref resource_data; }

struct pmd_morph_data
{
    struct morph_vertex
    {
        unsigned int idx;
        nya_math::vec3 pos;
    };

    enum morph_kind
    {
        morph_base=0,
        morph_brow,
        morph_eye,
        morph_mouth,
        morph_other
    };

    struct morph
    {
        morph_kind kind;
        std::string name;
        std::vector<morph_vertex> verts;
    };

    std::vector<morph> morphs;
};

struct pmd_phys_data
{
    enum shape_type
    {
        shape_sphere=0,
        shape_box=1,
        shape_capsule=2
    };

    enum object_type
    {
        object_static=0,
        object_dynamic=1,
        object_aligned=2
    };

    struct rigid_body
    {
        std::string name;
        int bone;
        unsigned char collision_group;
        unsigned short collision_mask;
        shape_type type;
        nya_math::vec3 size;
        nya_math::vec3 pos;
        nya_math::vec3 rot;
        float mass;
        float vel_attenuation;
        float rot_attenuation;
        float restriction;
        float friction;
        object_type mode;
    };

    std::vector<rigid_body> rigid_bodies;

    struct joint
    {
        std::string name;
        int rigid_src;
        int rigid_dst;

        nya_math::vec3 pos;
        nya_math::vec3 pos_max;
        nya_math::vec3 pos_min;
        nya_math::vec3 pos_spring;

        nya_math::vec3 rot;
        nya_math::vec3 rot_max;
        nya_math::vec3 rot_min;
        nya_math::vec3 rot_spring;
    };

    std::vector<joint> joints;
};

struct pmd_loader
{
public:
    struct pmd_material_params
    {
        float diffuse[4];
        float shininess;        
        float specular[3];
        float ambient[3];
    };

    struct vert
    {
        nya_math::vec3 pos;
        nya_math::vec3 normal;
        nya_math::vec2 tc;
        float bone_idx[2];
        float bone_weight;
    };

    struct pmd_bone
    {
        std::string name;
        int idx;
        int parent;
        nya_math::vec3 pos;
    };

public:
    static bool load(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name);

public:
    struct additional_data: public pmd_morph_data, pmd_phys_data {};
    static const additional_data *get_additional_data(const nya_scene::mesh &m);

public:
    static void get_tex_names(const std::string &src,std::string &base,std::string &spa,std::string &sph);
    static void load_textures(const std::string &path,const std::string &src,nya_scene::texture &base,nya_scene::texture &spa,nya_scene::texture &sph);
};
