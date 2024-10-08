//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "load_pmd.h"
#include "scene/mesh.h"
#include "memory/memory_reader.h"
#include "string_encoding.h"

namespace
{

struct add_data: public pmd_loader::additional_data, nya_scene::shared_mesh::additional_data
{
    const char *type() { return "pmd"; }
};

inline void read_vector(nya_math::vec3 &out,nya_memory::memory_reader &reader)
{
    out.x=reader.read<float>(),out.y=reader.read<float>(),out.z=-reader.read<float>();
}

inline void read_angle(nya_math::vec3 &out,nya_memory::memory_reader &reader)
{
    out.x=-reader.read<float>(),out.y=-reader.read<float>(),out.z=reader.read<float>();
}

}

bool pmd_loader::load(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name)
{
    nya_memory::memory_reader reader(data.get_data(),data.get_size());
    if(!reader.test("Pmd",3))
        return false;

    if(reader.read<float>()!=1.0f)
        return false;

    reader.skip(20+256); //name and comment

    typedef unsigned int uint;
    typedef unsigned short ushort;
    typedef unsigned char uchar;

    const uint vert_count=reader.read<uint>();
    const size_t pmd_vert_size=sizeof(float)*8+sizeof(ushort)*2+sizeof(uchar)*2;
    if(!reader.check_remained(pmd_vert_size*vert_count))
        return false;

    std::vector<vert> vertices(vert_count);
    for(size_t i=0;i<vertices.size();++i)
    {
        read_vector(vertices[i].pos,reader);
        read_vector(vertices[i].normal,reader);

        vertices[i].tc.x=reader.read<float>();
        vertices[i].tc.y=1.0f-reader.read<float>();

        vertices[i].bone_idx[0]=reader.read<ushort>();
        vertices[i].bone_idx[1]=reader.read<ushort>();
        vertices[i].bone_weight=reader.read<uchar>()/100.0f;

        reader.skip(1);
    }

    const uint ind_count=reader.read<uint>();
    const size_t ind_size=sizeof(ushort)*ind_count;
    if(!reader.check_remained(ind_size))
        return false;
    
    std::vector<uint16_t> indices(ind_count);
    const uint16_t *ind_src=(uint16_t *)data.get_data(reader.get_offset());
    std::vector<uint16_t> tmp(ind_count);
    for(uint32_t i=0;i<ind_count;i+=3)
    {
        indices[i]=ind_src[i];
        indices[i+1]=ind_src[i+2];
        indices[i+2]=ind_src[i+1];
    }
    res.vbo.set_index_data(indices.data(),nya_render::vbo::index2b,ind_count);
    reader.skip(ind_size);

    const uint mat_count=reader.read<uint>();
    if(!reader.check_remained(mat_count*(sizeof(pmd_material_params)+2+sizeof(uint)+20)))
        return false;

    res.groups.resize(mat_count);
    res.materials.resize(mat_count);

    std::string path(name);
    size_t p=path.rfind("/");
    if(p==std::string::npos)
        p=path.rfind("\\");
    if(p==std::string::npos)
        path.clear();
    else
        path.resize(p+1);

    std::vector<char> toon_idxs(mat_count);

    for(uint i=0,ind_offset=0;i<mat_count;++i)
    {
        nya_scene::shared_mesh::group &g=res.groups[i];

        const pmd_material_params params=reader.read<pmd_material_params>();

        toon_idxs[i]=reader.read<char>();
        const char edge_flag=reader.read<char>();

        g.name="mesh";
        g.offset=ind_offset;
        g.count=reader.read<uint>();
        g.material_idx=i;
        ind_offset+=g.count;

        const std::string tex_name((const char*)data.get_data(reader.get_offset()));
        reader.skip(20);

        std::string base_tex=tex_name;
        size_t pos=base_tex.find('*');
        if(pos!=std::string::npos)
            base_tex.resize(pos);

        nya_scene::material &m = res.materials[i];
        m.load("mmd.txt");

        nya_scene::texture base,spa,sph;
        pmd_loader::load_textures(path,base_tex,base,spa,sph);
        res.materials[i].set_texture("diffuse",base);
        res.materials[i].set_texture("env add",spa);
        res.materials[i].set_texture("env mult",sph);

        m.set_param("amb k",params.ambient[0],params.ambient[1],params.ambient[2],1.0f);
        m.set_param("diff k",params.diffuse[0],params.diffuse[1],params.diffuse[2],params.diffuse[3]);
        m.set_param("spec k",params.specular[0],params.specular[1],params.specular[2],params.shininess);

        if(!edge_flag)
            continue;

        res.groups.resize(res.groups.size()+1);
        nya_scene::shared_mesh::group &ge=res.groups.back();
        ge=res.groups[i];
        ge.name="edge";
        ge.material_idx=int(res.materials.size());
        res.materials.resize(res.materials.size()+1);
        nya_scene::material &me = res.materials.back();
        me.load("mmd_edge.txt");
    }

    const ushort bones_count=reader.read<ushort>();
    if(!bones_count || !reader.check_remained(bones_count*(20+sizeof(short)+5+sizeof(nya_math::vec3))))
        return false;

    std::vector<pmd_bone> bones(bones_count);
    for(int i=0;i<bones_count;++i)
    {
        pmd_bone &b=bones[i];
        b.idx=i;
        b.name=utf8_from_shiftjis(data.get_data(reader.get_offset()),20);
        reader.skip(20);
        b.parent=reader.read<short>();
        reader.skip(5); //child,kind,ik target

        read_vector(b.pos,reader);
    }

    for(int i=0;i<int(bones.size());++i)
    {
        bool had_sorted=false;
        for(int j=0;j<int(bones.size());++j)
        {
            const int p=bones[j].parent;
            if(p<=j)
                continue;

            had_sorted=true;
            std::swap(bones[j],bones[p]);
            for(int k=0;k<(int)bones.size();++k)
            {
                if(bones[k].parent==j)
                    bones[k].parent=p;
                else if(bones[k].parent==p)
                    bones[k].parent=j;
            }
        }

        if(!had_sorted)
            break;
    }

    std::vector<int> old_bones(bones_count);
    for(int i=0;i<bones_count;++i)
        old_bones[bones[i].idx]=i;

    for(int i=0;i<bones_count;++i)
    {
        const pmd_bone &b=bones[i];

        if(res.skeleton.add_bone(b.name.c_str(),b.pos,nya_math::quat(),b.parent,true)!=i)
        {
            nya_log::log()<<"pmd load error: invalid bone["<<old_bones[i]<<"]: "<<b.name.c_str()<<"\n";
            return false;
        }
    }

    const ushort iks_count=reader.read<ushort>();
    //if(!reader.check_remained(iks_count*()))
    //    return false;

    for(ushort i=0;i<iks_count;++i)
    {
        const short target=reader.read<short>();
        const short eff=reader.read<short>();
        const uchar link_count=reader.read<uchar>();
        const ushort count=reader.read<ushort>();
        const float k=reader.read<float>();

        const int ik=res.skeleton.add_ik(old_bones[target],old_bones[eff],count,k*nya_math::constants::pi);
        for(int j=0;j<link_count;++j)
        {
            const ushort link=reader.read<ushort>();
            const char *name=res.skeleton.get_bone_name(link);
            if(!name)
            {
                nya_log::log()<<"pmd load warning: invalid ik link\n";
                continue;
            }

            if(strcmp(name,"\xe5\xb7\xa6\xe3\x81\xb2\xe3\x81\x96")==0 || strcmp(name,"\xe5\x8f\xb3\xe3\x81\xb2\xe3\x81\x96")==0)
                res.skeleton.add_ik_link(ik,old_bones[link],nya_math::vec3(0.001f,0,0),nya_math::vec3(nya_math::constants::pi,0,0));
            else
                res.skeleton.add_ik_link(ik,old_bones[link]);
        }
    }

    add_data *ad=new add_data;
    res.add_data=ad;

    const ushort morphs_count=reader.read<ushort>();
    if(morphs_count>1)
        ad->morphs.resize(morphs_count-1);

    pmd_morph_data::morph base_morph;
    for(ushort i=0;i<ushort(ad->morphs.size());)
    {
        const std::string name=utf8_from_shiftjis(data.get_data(reader.get_offset()),20);
        reader.skip(20);
        const uint size=reader.read<uint>();
        const uchar type=reader.read<uchar>();

        pmd_morph_data::morph &m=type?ad->morphs[i]:base_morph;
        if(type)
            ++i;

        m.name=name;
        m.verts.resize(size);
        for(uint j=0;j<size;++j)
        {
            pmd_morph_data::morph_vertex &v=m.verts[j];

            const uint idx=reader.read<uint>();
            v.idx=type?base_morph.verts[idx].idx:idx;
            read_vector(v.pos,reader);
        }
    }
    
    const float tf=1.0f/bones_count;
    for(int i=0;i<vert_count;++i)
    {
        vert &v=vertices[i];
        for(int j=0;j<2;++j)
            v.bone_idx[j]=(v.bone_idx[j]+0.5f)*tf;
    }

    res.vbo.set_vertex_data(&vertices[0],sizeof(vertices[0]),vert_count);
    res.vbo.set_normals(3*sizeof(float));
    res.vbo.set_tc(0,6*sizeof(float),2);
    res.vbo.set_tc(1,8*sizeof(float),3); //skin info

    reader.skip(reader.read<uchar>()*sizeof(ushort));

    const uchar bone_groups_count=reader.read<uchar>();
    reader.skip(bone_groups_count*50);

    reader.skip(reader.read<uint>()*(sizeof(ushort)+sizeof(uchar)));

    if(!reader.get_remained())
        return true;

    if(reader.read<uchar>())
    {
        reader.skip(20);
        reader.skip(256);
        reader.skip(bones_count*20);

        if(morphs_count>1)
            reader.skip((morphs_count-1)*20);

        reader.skip(bone_groups_count*50);
    }

    char toon_names[10][100];
    for(int i=0;i<10;++i)
    {
        memcpy(toon_names[i],reader.get_data(),100);
        reader.skip(100);
    }

    for(uint i=0;i<mat_count;++i)
    {
        nya_scene::material &m=res.materials[i];

        bool loaded=false;
        nya_scene::texture tex;

        const char idx=toon_idxs[i];
        if(idx>=0 && idx<10)
        {
            const char *name=toon_names[idx];
            if(nya_resources::get_resources_provider().has((path+name).c_str()))
                loaded=tex.load((path+name).c_str());
            else
                loaded=tex.load(name);
        }

        if(!loaded)
            tex.build("\xff\xff\xff\xff",1,1,nya_render::texture::color_bgra);

        nya_render::texture rtex=tex.internal().get_shared_data()->tex;
        rtex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
        m.set_texture("toon",tex);
    }

    const uint rigid_bodies_count=reader.read<uint>();
    ad->rigid_bodies.resize(rigid_bodies_count);
    for(uint i=0;i<rigid_bodies_count;++i)
    {
        pmd_phys_data::rigid_body &rb=ad->rigid_bodies[i];
        rb.name=utf8_from_shiftjis(data.get_data(reader.get_offset()),20);
        reader.skip(20);
        rb.bone=reader.read<short>();
        if(rb.bone>=0)
            rb.bone=old_bones[rb.bone];

        rb.collision_group=reader.read<uchar>();
        rb.collision_mask=reader.read<ushort>();
        rb.type=(pmd_phys_data::shape_type)reader.read<uchar>();
        rb.size=reader.read<nya_math::vec3>();
        read_vector(rb.pos,reader);
        read_angle(rb.rot,reader);
        rb.mass=reader.read<float>();
        rb.vel_attenuation=reader.read<float>();
        rb.rot_attenuation=reader.read<float>();
        rb.restriction=reader.read<float>();
        rb.friction=reader.read<float>();
        rb.mode=(pmd_phys_data::object_type)reader.read<uchar>();
    }

    const uint joints_count=reader.read<uint>();
    ad->joints.resize(joints_count);
    for(uint i=0;i<joints_count;++i)
    {
        pmd_phys_data::joint &j=ad->joints[i];
        j.name=utf8_from_shiftjis(data.get_data(reader.get_offset()),20);
        reader.skip(20);
        j.rigid_src=reader.read<int>();
        j.rigid_dst=reader.read<int>();
        read_vector(j.pos,reader);
        read_angle(j.rot,reader);

        read_vector(j.pos_min,reader);
        read_vector(j.pos_max,reader);
        std::swap(j.pos_max.z,j.pos_min.z);

        read_angle(j.rot_min,reader);
        read_angle(j.rot_max,reader);
        std::swap(j.rot_max.x,j.rot_min.x);
        std::swap(j.rot_max.y,j.rot_min.y);

        j.pos_spring=reader.read<nya_math::vec3>();
        read_angle(j.rot_spring,reader);
    }

    return true;
}

const pmd_loader::additional_data *pmd_loader::get_additional_data(const nya_scene::mesh &m)
{
    if(!m.internal().get_shared_data().is_valid())
        return 0;

    nya_scene::shared_mesh::additional_data *d=m.internal().get_shared_data()->add_data;
    if(!d)
        return 0;

    const char *type=d->type();
    if(!type || strcmp(type,"pmd")!=0)
        return 0;

    return static_cast<pmd_loader::additional_data*>(static_cast<add_data*>(d));
}

void pmd_loader::get_tex_names(const std::string &src,std::string &base,std::string &spa,std::string &sph)
{
    std::string s[2];
    size_t pos=src.find('*');
    s[0]=src;
    if(pos!=std::string::npos)
    {
        s[0].resize(pos);
        s[1]=src.substr(pos+1);
    }

    for(int i=0;i<2;++i)
    {
        if(s[i].empty())
            continue;

        if(nya_resources::check_extension(s[i].c_str(),".spa"))
            spa=s[i];
        else if(nya_resources::check_extension(s[i].c_str(),".sph"))
            sph=s[i];
        else
            base=s[i];
    }
}

void pmd_loader::load_textures(const std::string &path,const std::string &src,nya_scene::texture &base,nya_scene::texture &spa,nya_scene::texture &sph)
{
    std::string base_name,spa_name,sph_name;
    get_tex_names(src,base_name,spa_name,sph_name);

    if(base_name.empty() || !base.load((path+base_name).c_str()))
        base.build("\xff\xff\xff\xff",1,1,nya_render::texture::color_rgba);

    if(sph_name.empty() || !sph.load((path+sph_name).c_str()))
        sph.build("\xff\xff\xff\xff",1,1,nya_render::texture::color_rgba);

    if(spa_name.empty() || !spa.load((path+spa_name).c_str()))
        spa.build("\x00\x00\x00\x00",1,1,nya_render::texture::color_rgba);
}

