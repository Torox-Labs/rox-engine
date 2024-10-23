//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "shared_resources.h"
#include "material.h"
#include "formats/math_expr_parser.h"
#include "material.h"
#include "transform.h"
#include "render/vbo.h"

namespace nya_scene
{

struct shared_particles
{
    struct bind { short from,to; };
    typedef std::vector<bind> var_binds;
    typedef std::vector<float> vars;

    struct sh_bind { short from; unsigned char to_idx,to_swizzle; };
    typedef std::vector<sh_bind> sh_binds;

    struct prm_bind { unsigned char from,from_swizzle; short to_idx; };
    typedef std::vector<prm_bind> prm_binds;

    struct param
    {
        std::string id, name;
        nya_math::vec4 value;
    };

    std::vector<param> params;

    struct tex
    {
        std::string id, name;
        texture_proxy value;
    };

    std::vector<tex> textures;

    typedef std::vector<std::pair<float,nya_math::vec4> > curve_points;

    struct curve
    {
        std::string id, name;
        curve_points points;
        static const int samples_count=128;
        nya_math::vec4 samples[samples_count];

        void sample(const curve_points &points);
    };

    std::vector<curve> curves;

    struct expression
    {
        short inout_idx;
        short bind_offset,bind_count;
        mutable nya_formats::math_expr_parser expr; //ToDo
    };

    struct function
    {
        std::string id;
        std::vector<std::string> in_out;
        std::vector<expression> expressions;
        var_binds binds;
        prm_binds param_binds;

        short get_inout_idx(const char *name) const;
        void update_binds(const std::vector<param> &params);
        void calculate(float *inout_buf) const; //inout_buf size must be equal in_out.size()
        static void link(const function &from,const function &to,var_binds &binds,const char *prefix="");
        static void link(const function &from,const material &to,sh_binds &binds);
        static void update_in(const float *buf_from,float *buf_to,const var_binds &binds);
    };

    std::vector<function> functions;

    short find_function(const char *id) const;

    struct particle
    {
        std::string name;
        material mat;
        mutable std::vector<material::param_array_proxy> params; //ToDo
        short init,update;
        short sort_key_offset;
        bool sort_ascending;

        nya_render::vbo mesh;
        short prim_count;
        short element_per_prim;
        bool prim_looped;

        var_binds init_update_binds;
        sh_binds update_sh_binds;

        void init_mesh_points(int count);
        void init_mesh_quads(int count);
        void init_mesh_quad_strip(int count);
        void init_mesh_line(int count);

        particle(): init(-1),update(-1),sort_key_offset(-1),prim_count(0),element_per_prim(0),prim_looped(false) {}
    };

    std::vector<particle> particles;

    struct emitter_particle_bind
    {
        var_binds init;
        var_binds update;
    };

    struct emitter
    {
        std::string id;
        short init,update;
        unsigned short fixed_fps;

        var_binds init_update_binds;
        std::vector<emitter_particle_bind> particle_binds;

        emitter(): init(-1),update(-1),fixed_fps(0) {}
    };

    std::vector<emitter> emitters;

    struct emitter_bind
    {
        short from,to;
        var_binds init_binds;
        var_binds update_binds;
    };

    std::vector<short> spawn;

    bool release()
    {
        for(size_t i=0;i<particles.size();++i)
            particles[i].mesh.release();

        functions.clear();
        particles.clear();
        emitters.clear();
        spawn.clear();

        return true;
    }
};

class particles: public scene_shared<shared_particles>
{
public:
    bool load(const char *name);
    void unload();

public:
    void create(const shared_particles &res);

public:
    void reset_time();
    void update(unsigned int dt);
    void draw(const char *pass_name=material::default_pass) const;

public:
    void set_pos(const nya_math::vec3 &pos) { m_transform.set_pos(pos); }
    void set_rot(const nya_math::quat &rot) { m_transform.set_rot(rot); }
    void set_rot(nya_math::angle_deg yaw,nya_math::angle_deg pitch,nya_math::angle_deg roll);
    void set_scale(float s) { m_transform.set_scale(s,s,s); }
    void set_scale(const nya_math::vec3 &s) { m_transform.set_scale(s.x,s.y,s.z); }

public:
    const nya_math::vec3 &get_pos() const { return m_transform.get_pos(); }
    const nya_math::quat &get_rot() const { return m_transform.get_rot(); }
    const nya_math::vec3 &get_scale() const { return m_transform.get_scale(); }

public:
    void spawn(const char *emitter_id);

public:
    int get_params_count() const;
    const char *get_param_name(int idx) const;
    const char *get_param_id(int idx) const;
    int get_param_idx(const char *name) const;

    void set_param(int idx,float f0,float f1=0.0f,float f2=0.0f,float f3=0.0f);
    void set_param(int idx,const nya_math::vec3 &v,float w=0.0f);
    void set_param(int idx,const nya_math::vec4 &v);

    void set_param(const char *name,float f0,float f1=0.0f,float f2=0.0f,float f3=0.0f);
    void set_param(const char *name,const nya_math::vec3 &v,float w=0.0f);
    void set_param(const char *name,const nya_math::vec4 &v);

    const nya_math::vec4 &get_param(int idx) const;
    const nya_math::vec4 &get_param(const char *name) const;

public:
    int get_textures_count() const;
    const char *get_texture_name(int idx) const;
    const char *get_texture_id(int idx) const;
    int get_texture_idx(const char *name) const;

public:
    unsigned int get_count() const;

public:
    particles(): m_time(0), m_need_update_params(false) {}
    particles(const char *name) { *this=particles(); load(name); }

public:
    static bool load_text(shared_particles &res,resource_data &data,const char* name);

private:
    void init();
    const shared_particles::function &get_function(int idx);
    int find_emitter_emitter_bind(int from,int to);
    void update_params(float *buf_to,const shared_particles::prm_binds &binds) const;

    void emit_particle(short emitter_idx,short particle_idx,int count);
    void spawn(short emitter_type,int count,short parent= -1);

private:
    unsigned int m_time;
    transform m_transform;

    struct emitter
    {
        bool dead;
        short type;
        short parent,parent_bind;
        std::vector<float> init_buf;
        std::vector<float> update_buf;

        unsigned int ref_count;
        unsigned int last_update_time;

        emitter(): dead(false),type(0),parent(-1),parent_bind(-1),ref_count(0),last_update_time(0) {}
    };

    std::vector<emitter> m_emitters;

    struct particle
    {
        std::vector<float> init_buf;
        unsigned int update_buf_size;
        unsigned int count;
        std::vector<float> update_bufs;
        std::vector<short> parent_emitters;
    };

    std::vector<particle> m_particles;

    struct spawn_emitter { short type,count,parent; };
    std::vector<spawn_emitter> m_spawn_emitters;

    std::vector<shared_particles::emitter_bind> m_emitter_emitter_binds;

    std::vector<nya_math::vec4> m_params;
    std::vector<nya_scene::texture_proxy> m_textures;
    bool m_need_update_params;

    nya_math::vec3 m_local_cam_pos;

private:
    static void time_func(float *a,float *r);
    static void get_dt_func(float *a,float *r);
    static void emit_func(float *a,float *r);
    static void spawn_func(float *a,float *r);
    static void curve_func(float *a,float *r);
    static void print_func(float *a,float *r);
    static void die_if_func(float *a,float *r);
    static void dist_to_cam(float *a,float *r);
    static void fade(float *a,float *r);

private:
    static particles *m_curr_particles;
    static bool m_curr_want_die;
    static bool m_in_emitter_init;
    static short m_curr_emitter_idx;
    static float m_dt;
};

}
