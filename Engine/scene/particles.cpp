//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "particles.h"
#include "camera.h"
#include "formats/text_parser.h"
#include "formats/string_convert.h"
#include "memory/invalid_object.h"
#include "stdlib.h"
#include "string.h"

namespace nya_scene
{

bool particles::load(const char *name)
{
    default_load_function(load_text);
    if(!scene_shared::load(name))
        return false;

    init();
    return true;
}

void particles::unload()
{
    scene_shared::unload();
    m_emitters.clear();
    m_params.clear();
    m_textures.clear();
    m_particles.clear();
}

void particles::create(const shared_particles &res)
{
    scene_shared::create(res);
    init();
}

void particles::init()
{
    m_emitter_emitter_binds.clear();

    m_params.resize(m_shared->params.size());
    for(int i=0;i<get_params_count();++i)
        m_params[i]=m_shared->params[i].value;

    m_textures.resize(m_shared->textures.size());
    for(int i=0;i<get_textures_count();++i)
        m_textures[i]=m_shared->textures[i].value;

    m_particles.resize(m_shared->particles.size());
    for(int i=0;i<(int)m_particles.size();++i)
    {
        const shared_particles::particle &p=m_shared->particles[i];
        m_particles[i].init_buf.resize(get_function(p.init).in_out.size());
        m_particles[i].update_buf_size=(unsigned int)get_function(p.update).in_out.size();
    }

    reset_time();
}

void particles::reset_time()
{
    m_time=0;
    m_emitters.clear();

    for(int i=0;i<(int)m_particles.size();++i)
    {
        particle &p=m_particles[i];
        p.count=0;
        p.update_bufs.clear();
        if(!p.init_buf.empty())
            memset(&p.init_buf[0],0,p.init_buf.size()*sizeof(p.init_buf[0]));
    }

    if(!m_shared.is_valid())
        return;

    m_curr_particles=this;
    for(int i=0;i<(int)m_shared->spawn.size();++i)
        spawn(m_shared->spawn[i],1);
}

void particles::update(unsigned int dt)
{
    if(dt>1000)
        dt=1000;

    m_local_cam_pos = m_transform.inverse_transform(get_camera().get_pos());

    m_curr_particles=this;
    m_time+=dt;
    m_dt=dt*0.001f;

    if(m_need_update_params)
    {
        for(m_curr_emitter_idx=0;m_curr_emitter_idx<(int)m_emitters.size();++m_curr_emitter_idx)
        {
            emitter &e=m_emitters[m_curr_emitter_idx];
            const shared_particles::emitter &se=m_shared->emitters[e.type];
            update_params(&e.update_buf[0],get_function(se.update).param_binds);
        }

        m_need_update_params=false;
    }

    for(int i=0;i<(int)m_spawn_emitters.size();++i)
        spawn(m_spawn_emitters[i].type,m_spawn_emitters[i].count,m_spawn_emitters[i].parent);
    m_spawn_emitters.clear();

    for(m_curr_emitter_idx=0;m_curr_emitter_idx<(int)m_emitters.size();++m_curr_emitter_idx)
    {
        emitter &e=m_emitters[m_curr_emitter_idx];
        const shared_particles::emitter &se=m_shared->emitters[e.type];
        m_curr_want_die=false;
        float *update_buf=&e.update_buf[0];

        if(e.dead)
            continue;

        if(e.parent>=0)
        {
            const emitter &pe=m_emitters[e.parent];
            shared_particles::function::update_in(&pe.update_buf[0],update_buf,m_emitter_emitter_binds[e.parent_bind].update_binds);
        }

        if(se.fixed_fps>0)
        {
            unsigned int frame_time=1000/se.fixed_fps;
            int update_count=int((m_time-e.last_update_time)/frame_time);
            if(update_count>0)
            {
                unsigned int restore_time=m_time;
                float restore_dt=m_dt;
                m_time=e.last_update_time;
                m_dt=frame_time*0.001f;
                for(int i=0;i<update_count && !e.dead;++i)
                {
                    m_time+=frame_time;
                    get_function(se.update).calculate(update_buf);
                    e.dead=m_curr_want_die;
                }
                e.last_update_time=m_time;
                m_time=restore_time;
                m_dt=restore_dt;
            }
        }
        else
        {
            get_function(se.update).calculate(update_buf);
            e.dead=m_curr_want_die;
        }
    }

    for(int i=0;i<(int)m_particles.size();++i)
    {
        const shared_particles::particle &sp=m_shared->particles[i];
        particle &p=m_particles[i];

        for(unsigned int j=0;j<p.count;)
        {
            m_curr_want_die=false;

            emitter &e=m_emitters[p.parent_emitters[j]];
            const shared_particles::emitter &se=m_shared->emitters[e.type];

            float *part_update_buf=&p.update_bufs[j*p.update_buf_size];
            shared_particles::function::update_in(&e.update_buf[0],part_update_buf,se.particle_binds[i].update);
            get_function(sp.update).calculate(part_update_buf);

            if(m_curr_want_die)
            {
                --p.count;
                --e.ref_count;
                if(j>=p.count)
                    break;

                memcpy(part_update_buf,&p.update_bufs[p.count*p.update_buf_size],p.update_buf_size*sizeof(float));
                p.parent_emitters[j]=p.parent_emitters[p.count];
            }
            else
                ++j;
        }

        if(sp.sort_key_offset>=0 && p.count>1)
        {
            float *part_update_buf=&p.update_bufs[0]+p.update_buf_size;
            for(unsigned int i=1;i<p.count;++i, part_update_buf+=p.update_buf_size)
            {
                float *buf=part_update_buf;
                for(unsigned int j=i;j>0;--j, buf-=p.update_buf_size)
                {
                    float *buf2=buf-p.update_buf_size;
                    if((buf[sp.sort_key_offset] <= buf2[sp.sort_key_offset]) ^ sp.sort_ascending)
                        break;

                    for(unsigned int k=0;k<p.update_buf_size;++k)
                        std::swap(buf[k],buf2[k]);
                    std::swap(p.parent_emitters[j],p.parent_emitters[j-1]);
                }
            }
        }
    }

    for(int i=0;i<(int)m_emitters.size();)
    {
        if(m_emitters[i].dead && m_emitters[i].ref_count==0)
        {
            if(m_emitters[i].parent>=0)
                --m_emitters[m_emitters[i].parent].ref_count;
            m_emitters.erase(m_emitters.begin()+i);
            for(int j=0;j<(int)m_particles.size();++j)
            {
                particle &p=m_particles[j];
                for(int k=0;k<(int)m_particles[j].count;++k)
                {
                    if(p.parent_emitters[k]>i)
                        --p.parent_emitters[k];
                }
            }

            for(int j=0;j<(int)m_emitters.size();++j)
            {
                if(m_emitters[j].parent>i)
                    --m_emitters[j].parent;
            }
        }
        else
            ++i;
    }
}

void particles::draw(const char *pass_name) const
{
    for(int i=0;i<(int)m_particles.size();++i)
    {
        const shared_particles::particle &sp=m_shared->particles[i];
        if(sp.mat.get_pass_idx(pass_name)<0)
            continue;

        const particle &p=m_particles[i];
        if(!p.count)
            continue;

        const float *part_update_buf=&p.update_bufs[0];
        int current=0;

        transform::set(m_transform);
        sp.mesh.bind();

        static std::vector<float *> params;
        params.resize(sp.params.size());
        for(size_t i=0;i<sp.params.size();++i)
            params[i]=sp.params[i]->get_buf();

        for(unsigned int j=0;j<p.count;++j,part_update_buf+=p.update_buf_size)
        {
            const int current_pidx=current*4;
            for(int k=0;k<(int)sp.update_sh_binds.size();++k)
            {
                const shared_particles::sh_bind &b=sp.update_sh_binds[k];
                if(current>=sp.params[b.to_idx]->get_count()) //ToDo: separate update uniform param binds
                    continue;

                params[b.to_idx][current_pidx+b.to_swizzle]=part_update_buf[b.from];
            }

            if(++current>=sp.prim_count)
            {
                sp.mat.internal().set(pass_name);
                sp.mesh.draw(current*sp.element_per_prim);
                sp.mat.internal().unset();
                current=0;

                if(sp.prim_looped)
                {
                    for(int k=0;k<(int)sp.update_sh_binds.size();++k)
                    {
                        const shared_particles::sh_bind &b=sp.update_sh_binds[k];
                        if(current>=sp.params[b.to_idx]->get_count()) //ToDo: separate update uniform param binds
                            continue;

                        params[b.to_idx][b.to_swizzle]=part_update_buf[b.from];
                    }
                    ++current;
                }
            }
        }

        if(current>0)
        {
            sp.mat.internal().set(pass_name);
            sp.mesh.draw(current*sp.element_per_prim);
            sp.mat.internal().unset();
        }

        sp.mesh.unbind();
    }
}

void particles::emit_particle(short emitter_idx,short particle_idx,int count)
{
    if(count<1)
        return;

    if(emitter_idx<0 || emitter_idx>=(short)m_emitters.size())
        return;

    if(particle_idx<0 || particle_idx>=(short)m_particles.size())
        return;

    emitter &e=m_emitters[emitter_idx];
    const shared_particles::emitter &se=m_shared->emitters[e.type];

    if(m_in_emitter_init && !e.init_buf.empty())
        shared_particles::function::update_in(&e.init_buf[0],&e.update_buf[0],se.init_update_binds);

    particle &p=m_particles[particle_idx];
    const shared_particles::particle &sp=m_shared->particles[particle_idx];
    if(!p.update_buf_size)
        return;

    p.update_bufs.resize((p.count+count)*p.update_buf_size,0.0f);
    p.parent_emitters.resize(p.count+count);

    float *part_init_buf=p.init_buf.empty()?0:&p.init_buf[0];
    float *part_update_buf=&p.update_bufs[p.count*p.update_buf_size];
    const float *update_buf=&e.update_buf[0];

    for(int i=0;i<count;++i,part_update_buf+=p.update_buf_size)
    {
        update_params(part_init_buf,get_function(sp.init).param_binds);
        shared_particles::function::update_in(update_buf,part_init_buf,se.particle_binds[particle_idx].init);
        get_function(sp.init).calculate(part_init_buf);
        shared_particles::function::update_in(part_init_buf,part_update_buf,sp.init_update_binds);
        p.parent_emitters[p.count+i]=emitter_idx;
    }

    p.count+=count;
    e.ref_count+=count;
}

void particles::spawn(short emitter_type,int count,short parent)
{
    if(count<1)
        return;

    if(emitter_type<0 || emitter_type>=(short)m_shared->emitters.size())
        return;

    const int curr_count=(short)m_emitters.size();
    m_emitters.resize(curr_count+count);

    const int curr=m_curr_emitter_idx;
    for(m_curr_emitter_idx=curr_count;m_curr_emitter_idx<(short)m_emitters.size();++m_curr_emitter_idx)
    {
        emitter &e=m_emitters[m_curr_emitter_idx];
        e.type=emitter_type;
        e.last_update_time=m_time;
        const shared_particles::emitter &se=m_shared->emitters[e.type];

        e.init_buf.resize(get_function(se.init).in_out.size());
        e.update_buf.resize(get_function(se.update).in_out.size());
        if(e.update_buf.empty())
            e.update_buf.resize(1); //removes unnecessary checks

        float *init_buf=e.init_buf.empty()?0:&e.init_buf[0];

        update_params(init_buf, get_function(se.init).param_binds);
        update_params(&e.update_buf[0],get_function(se.update).param_binds);

        if(parent>=0)
        {
            ++m_emitters[parent].ref_count;
            e.parent=parent;
            const emitter &pe=m_emitters[parent];
            e.parent_bind=find_emitter_emitter_bind(pe.type,e.type);
            if(e.parent_bind<0)
            {
                e.parent_bind=(short)m_emitter_emitter_binds.size();
                m_emitter_emitter_binds.resize(e.parent_bind+1);
                m_emitter_emitter_binds.back().from=pe.type;
                m_emitter_emitter_binds.back().to=e.type;
                const shared_particles::function &f=get_function(m_shared->emitters[pe.type].update);
                shared_particles::function::link(f,get_function(se.init),m_emitter_emitter_binds.back().init_binds,"parent.");
                shared_particles::function::link(f,get_function(se.update),m_emitter_emitter_binds.back().update_binds,"parent.");
            }

            shared_particles::function::update_in(&pe.update_buf[0],init_buf,m_emitter_emitter_binds[e.parent_bind].init_binds);
        }

        m_in_emitter_init=true;
        get_function(se.init).calculate(init_buf);
        m_in_emitter_init=false;
        shared_particles::function::update_in(init_buf,&e.update_buf[0],se.init_update_binds);
    }

    m_curr_emitter_idx=curr;
}

void particles::spawn(const char *emitter_id)
{
    if(!emitter_id || !m_shared.is_valid())
        return;

    for(int i=0;i<(int)m_shared->emitters.size();++i)
    {
        if(m_shared->emitters[i].id!=emitter_id)
            continue;

        spawn(i,1,-1);
        return;
    }
}

void particles::set_rot(nya_math::angle_deg yaw,nya_math::angle_deg pitch,nya_math::angle_deg roll)
{
    m_transform.set_rot(yaw,pitch,roll);
}

template<typename t>int get_idx(const t &array,std::string id)
{
    for(int i=0;i<(int)array.size();++i)
    {
        if(array[i].id==id)
            return i;
    }
    return -1;
}

template<typename t>int add_idx(t &array,std::string id)
{
    int idx=get_idx(array,id);
    if(idx<0)
    {
        idx=(int)array.size();
        array.resize(array.size()+1);
        array.back().id=id;
    }
    return idx;
}

const char *particles::get_param_name(int idx) const
{
    if(!m_shared.is_valid())
        return 0;

    if(idx<0 || idx>=get_params_count())
        return 0;

    return m_shared->params[idx].name.c_str();
}

const char *particles::get_param_id(int idx) const
{
    if(!m_shared.is_valid())
        return 0;

    if(idx<0 || idx>=get_params_count())
        return 0;

    return m_shared->params[idx].id.c_str();
}

int particles::get_params_count() const
{
    return (int)m_params.size();
}

int particles::get_param_idx(const char *name) const
{
    if(!name || !m_shared.is_valid())
        return -1;

    const int idx=get_idx(m_shared->params,name);
    if(idx>=0)
        return idx;

    for(int i=0;i<(int)m_shared->params.size();++i)
    {
        if(m_shared->params[i].name==name)
            return i;
    }

    return -1;
}

void particles::set_param(int idx,float f0,float f1,float f2,float f3)
{
    set_param(idx,nya_math::vec4(f0,f1,f2,f3));
}

void particles::set_param(int idx,const nya_math::vec3 &v,float w)
{
    set_param(idx,nya_math::vec4(v,w));
}

void particles::set_param(int idx,const nya_math::vec4 &p)
{
    if(idx<0 || idx>=get_params_count())
        return;

    if((m_params[idx]-p).length_sq()>0.001f)
        m_need_update_params=true;

    m_params[idx]=p;
}

void particles::set_param(const char *name,float f0,float f1,float f2,float f3)
{
    set_param(get_param_idx(name),f0,f1,f2,f3);
}

void particles::set_param(const char *name,const nya_math::vec3 &v,float w)
{
    set_param(get_param_idx(name),v,w);
}

void particles::set_param(const char *name,const nya_math::vec4 &v)
{
    set_param(get_param_idx(name),v);
}

const nya_math::vec4 &particles::get_param(int idx) const
{
    if(idx<0 || idx>=get_params_count())
        return nya_memory::invalid_object<nya_math::vec4>();

    return m_params[idx];
}

const nya_math::vec4 &particles::get_param(const char *name) const
{
    return get_param(get_param_idx(name));
}

unsigned int particles::get_count() const
{
    unsigned int count=0;
    for(int i=0;i<(int)m_particles.size();++i)
        count+=m_particles[i].count;

    return count;
}

int particles::get_textures_count() const
{
    return (int)m_textures.size();
}

const char *particles::get_texture_name(int idx) const
{
    if(!m_shared.is_valid())
        return 0;

    if(idx<0 || idx>=get_textures_count())
        return 0;

    return m_shared->textures[idx].name.c_str();
}

const char *particles::get_texture_id(int idx) const
{
    if(!m_shared.is_valid())
        return 0;

    if(idx<0 || idx>=get_textures_count())
        return 0;

    return m_shared->textures[idx].id.c_str();
}

int particles::get_texture_idx(const char *name) const
{
    if(!name || !m_shared.is_valid())
        return -1;

    const int idx=get_idx(m_shared->textures,name);
    if(idx>=0)
        return idx;

    for(int i=0;i<(int)m_shared->textures.size();++i)
    {
        if(m_shared->textures[i].name==name)
            return i;
    }

    return -1;
}

int particles::find_emitter_emitter_bind(int from,int to)
{
    for(int i=0;i<(int)m_emitter_emitter_binds.size();++i)
    {
        if(m_emitter_emitter_binds[i].from==from && m_emitter_emitter_binds[i].to==to)
            return i;
    }

    return -1;
}

const shared_particles::function &particles::get_function(int idx)
{
    if(!m_shared.is_valid() || idx<0)
        return nya_memory::invalid_object<shared_particles::function>();

    return m_shared->functions[idx];
}

short shared_particles::find_function(const char *id) const
{
    if(!id || !id[0] || functions.empty())
        return -1;

    return get_idx(functions,id);
}

void shared_particles::particle::init_mesh_points(int count)
{
    mesh.release();

    prim_count=count;
    element_per_prim=1;
    prim_looped=false;

    if(count<=0)
        return;

    //Note: 1-dimensional positions cause gl to crash

    std::vector<float> verts(prim_count*2);
    for(int i=0;i<prim_count;++i)
        verts[i*2]=float(i);

    mesh.set_vertices(0,2);
    mesh.set_element_type(nya_render::vbo::points);
    mesh.set_vertex_data(&verts[0],sizeof(float)*2,(unsigned int)prim_count);
}

void shared_particles::particle::init_mesh_line(int count)
{
    init_mesh_points(count);
    prim_looped=true;
    mesh.set_element_type(nya_render::vbo::line_strip);
}

namespace { struct quad_vert { float x,y,i,u,v; }; }

void shared_particles::particle::init_mesh_quads(int count)
{
    mesh.release();

    prim_count=count;
    element_per_prim=6;
    prim_looped=false;

    if(count<=0)
        return;

    std::vector<quad_vert> verts(prim_count*4);
    std::vector<unsigned short> inds(prim_count*6);

    for(int i=0,v=0,idx=0;i<prim_count*6;i+=6,v+=4,++idx)
    {
        for(int j=0;j<4;++j)
        {
            quad_vert &vert = verts[v+j];

            vert.x=j>1?-1.0f:1.0f, vert.y=j%2?1.0f:-1.0f;
            vert.u=0.5f*(vert.x+1.0f);
            vert.v=0.5f*(vert.y+1.0f);
            vert.i=float(idx);
        }

        const short quad_inds[]={0,1,2,2,1,3};
        for(int j=0;j<6;++j)
            inds[i+j]=v+quad_inds[j];
    }

    mesh.set_tc(0,sizeof(float)*3,2);
    mesh.set_vertex_data(&verts[0],sizeof(quad_vert),(unsigned int)verts.size());
    mesh.set_index_data(&inds[0],nya_render::vbo::index2b,(unsigned int)inds.size());
}

void shared_particles::particle::init_mesh_quad_strip(int count)
{
    mesh.release();

    prim_count=count;
    element_per_prim=2;
    prim_looped=true;

    if(count<=0)
        return;

    std::vector<nya_math::vec2> verts(count*2);
    for(int i=0;i<count*2;++i)
        verts[i].set(i%2?-1.0f:1.0f,float(i/2));

    mesh.set_vertices(0,2);
    mesh.set_element_type(nya_render::vbo::triangle_strip);
    mesh.set_vertex_data(&verts[0],sizeof(nya_math::vec2),(unsigned int)verts.size());
}

short shared_particles::function::get_inout_idx(const char *name) const
{
    if(!name)
        return -1;

    for(short i=0;i<(short)in_out.size();++i)
    {
        if(in_out[i]==name)
            return i;
    }
    return -1;
}

int get_swizzle(const std::string &name,unsigned char (&swizzle_indices)[4])
{
    const size_t dot=name.rfind('.');
    const int count=int(name.length()-dot-1);
    if(dot==std::string::npos || count>4)
        return 0;

    memset(swizzle_indices,-1,4);
    for(int i=0;i<count;++i)
    {
        switch(name[dot+1+i])
        {
            case 'x': case 'r': swizzle_indices[i]=0; break;
            case 'y': case 'g': swizzle_indices[i]=1; break;
            case 'z': case 'b': swizzle_indices[i]=2; break;
            case 'w': case 'a': swizzle_indices[i]=3; break;
            default:
                return 0;
        }
    }
    return count;
}

void shared_particles::function::update_binds(const std::vector<param> &params)
{
    binds.clear();
    for(int i=0;i<(int)expressions.size();++i)
    {
        shared_particles::expression &e=expressions[i];
        e.bind_offset=(short)binds.size();

        for(int j=0;j<e.expr.get_vars_count();++j)
        {
            const char *name=e.expr.get_var_name(j);
            short idx=get_inout_idx(name);
            if(idx<0)
                idx=(short)in_out.size(),in_out.push_back(name);

            bind b;
            b.to=(short)j,b.from=idx;
            binds.push_back(b);
        }

        e.bind_count=(short)binds.size()-e.bind_offset;
    }

    param_binds.clear();
    for(int i=0;i<(int)in_out.size();++i)
    {
        prm_bind b;
        b.to_idx=i;

        unsigned char sw[4];
        const int sw_count=get_swizzle(in_out[i],sw);
        if(!sw_count)
        {
            for(unsigned char j=0;j<(unsigned char)params.size();++j)
            {
                if(in_out[i]==params[j].id)
                {
                    b.from=j;
                    b.from_swizzle=0;
                    param_binds.push_back(b);
                    break;
                }
            }
            continue;
        }

        std::string name=in_out[i].substr(0,in_out[i].length()-sw_count-1);
        for(unsigned char j=0;j<(unsigned char)params.size();++j)
        {
            if(name==params[j].id)
            {
                for(int i=0;i<sw_count;++i)
                {
                    b.from=j,b.from_swizzle=sw[i];
                    param_binds.push_back(b);
                }
                break;
            }
        }
    }
}

void shared_particles::function::calculate(float *inout_buf) const
{
    for(size_t i=0;i<expressions.size();++i)
    {
        const expression &e=expressions[i];
        float *vars=e.expr.get_vars();
        for(int j=e.bind_offset;j<e.bind_offset+e.bind_count;++j)
            vars[binds[j].to]=inout_buf[binds[j].from];

        inout_buf[e.inout_idx]=e.expr.calculate();
    }
}

void shared_particles::function::link(const function &from,const function &to,var_binds &binds,const char *prefix)
{
    binds.clear();

    std::string p(prefix?prefix:"");

    for(short i=0;i<(short)to.in_out.size();++i) //ToDo: bind 'in' only?
    {
        if(to.in_out[i].empty())
            continue;

        for(short j=0;j<(short)from.in_out.size();++j)
        {
            if(to.in_out[i]==p+from.in_out[j])
            {
                bind b;
                b.from=j,b.to=i;
                binds.push_back(b);
                break;
            }
        }
    }
}

void shared_particles::function::link(const function &from,const material &to,sh_binds &binds)
{
    binds.clear();

    for(short i=0;i<(short)from.in_out.size();++i)
    {
        sh_bind b;
        b.from=i;

        unsigned char sw[4];
        const int sw_count=get_swizzle(from.in_out[i],sw);
        if(!sw_count)
        {
            const int pidx=to.get_param_idx(from.in_out[i].c_str());
            if(pidx<0)
                continue;

            b.to_idx=(unsigned char)pidx,b.to_swizzle=0;
            binds.push_back(b);
            continue;
        }

        std::string name=from.in_out[i].substr(0,from.in_out[i].length()-sw_count-1);
        const int pidx=to.get_param_idx(name.c_str());
        if(pidx<0)
            continue;

        for(int i=0;i<sw_count;++i)
        {
            b.to_idx=(unsigned char)(pidx),b.to_swizzle=sw[i];
            binds.push_back(b);
        }
    }
}

void shared_particles::function::update_in(const float *buf_from,float *buf_to,const var_binds &binds)
{
    for(size_t i=0;i<binds.size();++i)
    {
        const bind &b=binds[i];
        buf_to[b.to]=buf_from[b.from];
    }
}

void particles::update_params(float *buf_to,const shared_particles::prm_binds &binds) const
{
    for(int i=0;i<(int)binds.size();++i)
        buf_to[binds[i].to_idx]=(&m_params[binds[i].from].x)[binds[i].from_swizzle];
}

static bool compare_curve_points(shared_particles::curve_points::const_reference a,
                                 shared_particles::curve_points::const_reference b)
{
    return a.first<b.first;
}

void shared_particles::curve::sample(const curve_points &p)
{
    points=p;

    if(points.empty())
    {
        for(int i=0;i<samples_count;++i)
            samples[i]=nya_math::vec4();
        return;
    }

    if(points.size()==1)
    {
        for(int i=0;i<samples_count;++i)
            samples[i]=points.front().second;
        return;
    }

    std::sort(points.begin(),points.end(),compare_curve_points);

    for(int i=1;i<(int)points.size();++i)
    {
        const int idx0=int(points[i-1].first*(samples_count-1));
        const int idx1=int(points[i].first*(samples_count-1));

        for(int j=idx0<0?0:idx0;j<idx1 && j<samples_count;++j)
        {
            const float k=(j/float(samples_count-1) - points[i-1].first) / (points[i].first - points[i-1].first);
            samples[j]=nya_math::vec4::lerp(points[i-1].second,points[i].second, k);
        }
    }

    const int idxf=int(points.front().first*(samples_count-1));
    for(int i=0;i<idxf;++i)
        samples[i]=points.front().second;

    const int idxl=int(points.back().first*(samples_count-1));
    for(int i=idxl;i<samples_count;++i)
        samples[i]=points.back().second;
}

namespace { typedef std::list<nya_formats::text_parser> parsers_list; }

static bool load_includes(parsers_list &list,parsers_list::iterator current,const char *name)
{
    nya_formats::text_parser &parser=*current;
    for(int i=0;i<parser.get_sections_count();++i)
    {
        if(parser.is_section_type(i,"include"))
        {
            const char *sname=parser.get_section_name(i);
            if(!sname || !sname[0])
            {
                log()<<"unable to load include in particles "<<name<<": invalid filename\n";
                return false;
            }

            std::string path(name?name:"");
            size_t p=path.rfind("/");
            if(p==std::string::npos)
                p=path.rfind("\\");

            if(p==std::string::npos)
                path.clear();
            else
                path.resize(p+1);

            path.append(sname);

            nya_resources::resource_data *file_data=nya_resources::get_resources_provider().access(path.c_str());
            if(!file_data)
            {
                log()<<"unable to load include resource in particles "<<name<<": unable to access resource "<<path.c_str()<<"\n";
                return false;
            }

            const size_t data_size=file_data->get_size();
            nya_memory::tmp_buffer_scoped include_data(data_size);
            file_data->read_all(include_data.get_data());
            file_data->release();

            parsers_list::iterator it=current;
            ++it;
            it=list.insert(it,nya_formats::text_parser());

            if(!it->load_from_data((char *)include_data.get_data(),include_data.get_size()))
            {
                log()<<"unable to load include in particles "<<name<<": unknown format or invalid data in "<<path.c_str()<<"\n";
                return false;
            }

            load_includes(list,it,name);
        }
    }

    return true;
}

bool particles::load_text(shared_particles &res,resource_data &data,const char* name)
{
    typedef std::list<nya_formats::text_parser> parsers_list;
    parsers_list parsers(1);
    if(!parsers.back().load_from_data((char *)data.get_data(),data.get_size()))
        return false;

    if(!load_includes(parsers,parsers.begin(),name))
        return false;

    typedef std::map<std::string,std::string> textures_map;
    textures_map textures;
    textures_map texture_names;

    for (parsers_list::reverse_iterator it=parsers.rbegin();it!=parsers.rend();++it)
    {
        const nya_formats::text_parser &parser=*it;
        for(int i=0;i<parser.get_sections_count();++i)
        {
            if(parser.is_section_type(i,"emitter"))
            {
                const char *sname=parser.get_section_name(i);
                if(!sname || !sname[0])
                    continue;

                add_idx(res.emitters,sname);
                continue;
            }

            if(parser.is_section_type(i,"param"))
            {
                const char *sname=parser.get_section_name(i);
                if(!sname || !sname[0])
                    continue;

                const int idx=add_idx(res.params,sname);
                res.params[idx].value=parser.get_section_value_vec4(i);

                const char *pname=parser.get_section_name(i,1);
                if(pname && pname[0])
                    res.params[idx].name=pname;
                continue;
            }

            if(parser.is_section_type(i,"curve"))
            {
                const char *sname=parser.get_section_name(i);
                if(!sname || !sname[0])
                    continue;

                shared_particles::curve_points points;
                for(int j=0;j<parser.get_subsections_count(i);++j)
                {
                    const float a=(float)atof(parser.get_subsection_type(i,j));
                    points.push_back(std::make_pair(a,parser.get_subsection_value_vec4(i,j)));
                }

                const int idx=add_idx(res.curves,sname);
                res.curves[idx].sample(points);

                const char *pname=parser.get_section_name(i,1);
                if(pname && pname[0])
                res.curves[idx].name=pname;
                continue;
            }

            if(parser.is_section_type(i,"texture"))
            {
                const char *sname=parser.get_section_name(i);
                if(!sname || !sname[0])
                    continue;

                textures[sname]=parser.get_section_value(i);

                const char *pname=parser.get_section_name(i,1);
                if(pname && pname[0])
                    texture_names[sname]=pname;
                continue;
            }
        }
    }

    for(textures_map::const_iterator it=textures.begin();it!=textures.end();++it)
    {
        res.textures.resize(res.textures.size()+1);
        res.textures.back().id=it->first;
        res.textures.back().name=texture_names[it->first];
        res.textures.back().value=texture_proxy(it->second.c_str());
    }

    for (parsers_list::reverse_iterator it=parsers.rbegin();it!=parsers.rend();++it)
    {
        const nya_formats::text_parser &parser=*it;
        for(int i=0;i<parser.get_sections_count();++i)
        {
            const char *sname=parser.get_section_name(i);

            if(parser.is_section_type(i,"function"))
            {
                if(!sname || !sname[0])
                    continue;

                if(res.find_function(sname)>=0)
                {
                    log()<<"already defined function '"<<sname<<"' when loading particles '"<<name<<"'\n";
                    continue;
                }

                shared_particles::function f;
                f.id.assign(sname);

                f.expressions.resize(parser.get_subsections_count(i));
                for(int j=0;j<(int)f.expressions.size();++j)
                {
                    const char *fname=parser.get_subsection_type(i,j);
                    const char *fvalue=parser.get_subsection_value(i,j);
                    if(!fvalue || !fvalue[0])
                        fvalue=fname,fname="";

                    int idx=f.get_inout_idx(fname);
                    if(idx<0)
                        idx=(int)f.in_out.size(),f.in_out.push_back(fname);

                    f.expressions[j].inout_idx=idx;
                    nya_formats::math_expr_parser &e=f.expressions[j].expr;

                    e.set_function("time",1,1,time_func);
                    e.set_function("get_dt",0,1,get_dt_func);
                    e.set_function("print",1,1,print_func);
                    e.set_function("die_if",1,1,die_if_func);
                    e.set_function("dist_to_cam",3,1,dist_to_cam);
                    e.set_function("fade",4,1,fade);

                    //ToDo: warn on var name collisions

                    if(strstr(fvalue,"spawn"))
                    {
                        e.set_function("spawn",2,1,spawn_func);
                        for(int k=0;k<(int)res.emitters.size();++k)
                            e.set_constant(res.emitters[k].id.c_str(),float(k));
                    }

                    if(strstr(fvalue,"emit"))
                    {
                        e.set_function("emit",2,1,emit_func);
                        for(int k=0;k<(int)res.particles.size();++k)
                            e.set_constant(res.particles[k].name.c_str(),float(k));
                    }

                    if(strstr(fvalue,"curve"))
                    {
                        e.set_function("curve",2,1,curve_func);
                        for(int k=0;k<(int)res.curves.size();++k)
                        {
                            e.set_constant(res.curves[k].id.c_str(),float(k*4));
                            char rgba[]="rgba",xyzw[]="xyzw";
                            for(int i=0;i<4;++i)
                            {
                                e.set_constant((res.curves[k].id+'.'+rgba[i]).c_str(),float(k*4+i));
                                e.set_constant((res.curves[k].id+'.'+xyzw[i]).c_str(),float(k*4+i));
                            }
                        }
                    }

                    if(!e.parse(fvalue))
                    {
                        log()<<"invalid expression '"<<fvalue<<"' in function '"<<sname<<"' when loading particles '"<<name<<"'\n";
                        return false;
                    }
                }

                f.update_binds(res.params);
                res.functions.push_back(f);
                continue;
            }

            if(parser.is_section_type(i,"particle"))
            {
                shared_particles::particle p;

                const char *mat=parser.get_subsection_value(i,"material");
                if(mat && mat[0] && !p.mat.load(mat))
                {
                    log()<<"invalid material when loading particles '"<<name<<"'\n";
                    return false;
                }

                const char *sh=parser.get_subsection_value(i,"shader");
                if(sh && sh[0])
                {
                    shader s;
                    if(!s.load(sh))
                    {
                        log()<<"invalid shader when loading particles '"<<name<<"'\n";
                        return false;
                    }

                    p.mat.get_default_pass().set_shader(s);
                }

                p.name.assign(sname?sname:"");
                for(int j=0;j<parser.get_subsections_count(i);++j)
                {
                    const std::string type(parser.get_subsection_type(i,j));
                    const char *value=parser.get_subsection_value(i,j);

                    if(type=="init")
                    {
                        p.init=res.find_function(value);
                        if(p.init<0)
                        {
                            log()<<"unable to find init function "<<value<<" in particle "<<sname<<"\n";
                            return false;
                        }
                        continue;
                    }

                    if(type=="update")
                    {
                        p.update=res.find_function(value);
                        if(p.update<0)
                        {
                            log()<<"unable to find update function "<<value<<" in particle "<<sname<<"\n";
                            return false;
                        }
                        continue;
                    }

                    if(type=="material" || type=="shader")
                        continue;

                    if(type=="blend")
                    {
                        nya_render::state &s=p.mat.get_default_pass().get_state();
                        s.blend=nya_formats::blend_mode_from_string(value,s.blend_src,s.blend_dst);
                    }

                    if(type=="zwrite")
                    {
                        nya_render::state &s=p.mat.get_default_pass().get_state();
                        s.zwrite=parser.get_subsection_value_bool(i,j);
                    }

                    if(type=="depth_test")
                    {
                        nya_render::state &s=p.mat.get_default_pass().get_state();
                        s.depth_test=parser.get_subsection_value_bool(i,j);
                    }

                    if(type=="quads.count")
                    {
                        p.init_mesh_quads(parser.get_subsection_value_int(i,j));
                        continue;
                    }

                    if(type=="quad_strip.count")
                    {
                        p.init_mesh_quad_strip(parser.get_subsection_value_int(i,j));
                        continue;
                    }

                    if(type=="points.count")
                    {
                        p.init_mesh_points(parser.get_subsection_value_int(i,j));
                        continue;
                    }

                    if(type=="line.count")
                    {
                        p.init_mesh_line(parser.get_subsection_value_int(i,j));
                        continue;
                    }

                    int idx=p.mat.get_texture_idx(type.c_str());
                    if(idx>=0)
                    {
                        const int tidx=get_idx(res.textures,value);
                        if(tidx>=0)
                            p.mat.set_texture(idx,res.textures[tidx].value);
                        else
                            p.mat.set_texture(idx,texture(value));
                        continue;
                    }

                    idx=p.mat.get_param_idx(type.c_str());
                    if(idx>=0)
                    {
                        p.mat.set_param(idx,parser.get_subsection_value_vec4(i,j));
                        continue;
                    }
                }

                if(p.init>=0 && p.update>=0)
                    shared_particles::function::link(res.functions[p.init],res.functions[p.update],p.init_update_binds);

                if(p.update>=0)
                {
                    shared_particles::function::link(res.functions[p.update],p.mat,p.update_sh_binds);

                    const char *sort=parser.get_subsection_value(i,"sort_asc");
                    if(sort && sort[0])
                    {
                        p.sort_key_offset=res.functions[p.update].get_inout_idx(sort);
                        p.sort_ascending=true;
                    }
                    else if((sort=parser.get_subsection_value(i,"sort_desc")) && sort[0])
                    {
                        p.sort_key_offset=res.functions[p.update].get_inout_idx(sort);
                        p.sort_ascending=false;
                    }
                }

                p.params.resize(p.mat.get_params_count());
                for(int j=0;j<p.mat.get_params_count();++j)
                {
                    p.params[j]=p.mat.get_param_array(j);
                    if(p.params[j].is_valid())
                        continue;

                    p.params[j].create();
                    p.params[j]->set_count(1);
                    const material::param_proxy &pp=p.mat.get_param(j);
                    if(pp.is_valid())
                        p.params[j]->set(0,pp.get());

                    p.mat.set_param_array(j,p.params[j]);
                }

                res.particles.push_back(p);
                continue;
            }

            if(parser.is_section_type(i,"emitter"))
            {
                if(!sname || !sname[0])
                    continue;

                int idx=add_idx(res.emitters,sname);
                shared_particles::emitter &e=res.emitters[idx];

                for(int j=0;j<parser.get_subsections_count(i);++j)
                {
                    const std::string type(parser.get_subsection_type(i,j));
                    const char *value=parser.get_subsection_value(i,j);

                    if(type=="init")
                    {
                        e.init=res.find_function(value);
                        if(e.init<0)
                        {
                            log()<<"unable to find init function "<<value<<" in emitter "<<sname<<"\n";
                            return false;
                        }
                        continue;
                    }

                    if(type=="update")
                    {
                        e.update=res.find_function(value);
                        if(e.update<0)
                        {
                            log()<<"unable to find update function "<<value<<" in emitter "<<sname<<"\n";
                            return false;
                        }
                        continue;
                    }

                    if(type=="fixed_fps")
                    {
                        e.fixed_fps=parser.get_subsection_value_int(i,j);
                        continue;
                    }
                }

                if(e.init>=0 && e.update>=0)
                {
                    //pass all init inout to update //ToDo?
                    for(int i=0;i<(int)res.functions[e.init].in_out.size();++i)
                    {
                        const char *name=res.functions[e.init].in_out[i].c_str();
                        const int idx=res.functions[e.update].get_inout_idx(name);
                        if(idx<0)
                            res.functions[e.update].in_out.push_back(name);
                    }

                    shared_particles::function::link(res.functions[e.init],res.functions[e.update],e.init_update_binds);
                }

                e.particle_binds.resize(res.particles.size());
                for(int i=0;i<(int)res.particles.size();++i)
                {
                    shared_particles::emitter_particle_bind &b=e.particle_binds[i];
                    const shared_particles::particle &p=res.particles[i];
                    if(p.init>=0 && e.update>=0)
                        shared_particles::function::link(res.functions[e.update],res.functions[p.init],b.init,"parent.");

                    if(p.update>=0 && e.update>=0)
                        shared_particles::function::link(res.functions[e.update],res.functions[p.update],b.update,"parent.");
                }

                continue;
            }

            if(parser.is_section_type(i,"spawn"))
            {
                if(!sname || !sname[0])
                {
                    log()<<"invalid emitter in spawn when loading particles '"<<name<<"'\n";
                    return false;
                }

                int idx=get_idx(res.emitters,sname);
                if(idx<0)
                {
                    log()<<"invalid emitter '"<<sname<<"' in spawn when loading particles '"<<name<<"'\n";
                    return false;
                }

                res.spawn.push_back(idx);
                continue;
            }
        }
    }

    return true;
}

particles *particles::m_curr_particles=0;
bool particles::m_curr_want_die=false;
bool particles::m_in_emitter_init=false;
short particles::m_curr_emitter_idx= -1;
float particles::m_dt=0.0f;

void particles::time_func(float *a,float *r) { r[0] = (m_curr_particles->m_time % int(1000/a[0]))*(0.001f*a[0]); }
void particles::get_dt_func(float *a,float *r) { r[0] = m_dt; }

void particles::emit_func(float *a,float *r)
{
    const int count=(int)lroundf(a[1]);
    if(count<1)
    {
        r[0]=0.0f;
        return;
    }

    m_curr_particles->emit_particle(m_curr_emitter_idx,short(a[0]),count);
    r[0]=float(count);
}

void particles::spawn_func(float *a,float *r)
{
    const int count=(int)lroundf(a[1]);
    if(count<1)
    {
        r[0]=0.0f;
        return;
    }

    spawn_emitter e;
    e.type=int(a[0]);
    e.count=count;
    e.parent=m_curr_emitter_idx;
    m_curr_particles->m_spawn_emitters.push_back(e);
    r[0]=float(count);
}

void particles::curve_func(float *a,float *r)
{
    int sidx=int(a[1]*shared_particles::curve::samples_count);
    if(sidx>=shared_particles::curve::samples_count)
        sidx=shared_particles::curve::samples_count-1;
    else if(sidx<0)
        sidx=0;

    const unsigned int a0=(unsigned int)(a[0]);
    const unsigned int idx=a0/4;
    const std::vector<shared_particles::curve> &curves=m_curr_particles->get_shared_data()->curves;
    r[0]=idx>=curves.size()?0.0f:(&curves[idx].samples[sidx].x)[a0%4];
}

void particles::print_func(float *a,float *r) { log()<<"particle print: "<<a[0]<<"\n"; r[0]=a[0]; }
void particles::die_if_func(float *a,float *r) { r[0]=float(m_curr_want_die || (m_curr_want_die=a[0]>0.0f)); }
void particles::dist_to_cam(float *a,float *r) { r[0]=(m_curr_particles->m_local_cam_pos - nya_math::vec3(a[0],a[1],a[2])).length(); }
void particles::fade(float *a,float *r) { r[0]=nya_math::fade(a[0],a[1],a[2],a[3]); }

}
