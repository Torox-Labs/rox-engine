//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "load_pmd.h"
#include "math/quaternion.h"

namespace nya_memory { class memory_reader; class tmp_buffer_ref; }
namespace nya_scene { class mesh; struct shared_mesh; typedef nya_memory::tmp_buffer_ref resource_data; }

struct pmx_loader
{
public:
    struct pmx_header
    {
        char text_encoding;
        char extended_uv;
        char index_size;
        char texture_idx_size;
        char material_idx_size;
        char bone_idx_size;
        char morph_idx_size;
        char rigidbody_idx_size;
    };

    struct vert
    {
        nya_math::vec3 pos;
        float morph_idx;
        nya_math::vec3 normal;
        nya_math::vec2 tc;
        float bone_idx[4];
        float bone_weight[4];
    };

    struct pmx_material_params
    {
        nya_math::vec4 diffuse;
        nya_math::vec3 specular;
        float shininess;
        nya_math::vec3 ambient;
    };

    struct pmx_edge_params
    {
        nya_math::vec4 color;
        float width;
    };

    struct pmx_bone
    {
        std::string name;
        int idx;
        int parent;
        int order;
        nya_math::vec3 pos;

        struct
        {
            bool has_pos;
            bool has_rot;
            int src_idx;
            float k;
        } bound;

        struct ik_link
        {
            int idx;
            bool has_limits;
            nya_math::vec3 from;
            nya_math::vec3 to;
        };

        struct
        {
            bool has;
            int eff_idx;
            int count;
            float k;
            
            std::vector<ik_link> links;
        } ik;
    };

    static bool compare_order(const pmx_bone &a,const pmx_bone &b) { return a.order<b.order; }

    template<typename t> class flag
    {
    private:
        t m_data;

    public:
        flag(t data) { m_data=data; }
        bool c(t mask) const { return (m_data & mask)!=0; }
    };

public:
    static bool load(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name);

    static void set_load_textures(bool load) { m_load_textures = load; }

public:
    enum morph_type
    {
        morph_type_group=0,
        morph_type_vertex=1,
        morph_type_bone=2,
        morph_type_uv=3,
        morph_type_add_uv1=4,
        morph_type_add_uv2=5,
        morph_type_add_uv3=6,
        morph_type_add_uv4=7,
        morph_type_material=8
    };

    struct additional_data: public pmd_phys_data
    {
        struct morph
        {
            pmd_morph_data::morph_kind kind;
            morph_type type;
            std::string name;
            int idx;
        };
        std::vector<morph> morphs;

        struct group_morph { std::vector<std::pair<int,float> > morphs; };
        std::vector<group_morph> group_morphs;

        struct vert_morph { std::vector<pmd_morph_data::morph_vertex> verts; };
        std::vector<vert_morph> vert_morphs;
        int mvert_count,vert_tex_w,vert_tex_h;

        struct morph_bone { unsigned int idx; nya_math::vec3 pos; nya_math::quat rot; };
        struct bone_morph { std::vector<morph_bone> bones; };
        std::vector<bone_morph> bone_morphs;

        struct morph_uv { unsigned int idx; nya_math::vec2 tc; };
        struct uv_morph { std::vector<morph_uv> verts; };
        std::vector<uv_morph> uv_morphs;

        struct morph_mat
        {
            int mat_idx;
            enum op_type { op_mult=0,op_add=1 } op;
            pmx_material_params params;
            pmx_edge_params edge_params;
            nya_math::vec4 tex_color;
            nya_math::vec4 sp_tex_color;
            nya_math::vec4 toon_tex_color;
        };

        struct mat_morph { std::vector<morph_mat> mats; };
        std::vector<mat_morph> mat_morphs;

        struct mat
        {
            std::string tex_diffuse;
            std::string tex_toon;
            std::string tex_env;

            pmx_material_params params;
            pmx_edge_params edge_params;
            int edge_group_idx;
            bool backface_cull;
            bool shadow_ground;
            bool shadow_cast;
            bool shadow_receive;
        };

        std::vector<mat> materials;
    };

    static const additional_data *get_additional_data(const nya_scene::mesh &m);

private:
    static bool m_load_textures;
};
