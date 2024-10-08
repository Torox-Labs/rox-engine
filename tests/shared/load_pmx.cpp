//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "load_pmx.h"
#include "scene/mesh.h"
#include "memory/memory_reader.h"
#include "string_encoding.h"

#include "resources/resources.h"

namespace
{

struct add_data: public pmx_loader::additional_data, nya_scene::shared_mesh::additional_data
{
    const char *type() { return "pmx"; }
};

int read_idx(nya_memory::memory_reader &reader,int size)
{
    switch(size)
    {
        case 1: return reader.read<int8_t>();
        case 2: return reader.read<int16_t>();
        case 4: return reader.read<int32_t>();
    }

    return 0;
}

inline std::string read_string(nya_memory::memory_reader &reader,char encoding)
{
    const int name_len=reader.read<int>();
    std::string str=encoding?std::string((const char*)reader.get_data(),name_len):
    utf8_from_utf16le(reader.get_data(),name_len);
    reader.skip(name_len);
    return str;
}

inline void skip_string(nya_memory::memory_reader &reader) { reader.skip(reader.read<int>()); }

template<typename t> void load_vertex_morph(nya_memory::memory_reader &reader,pmx_loader::additional_data::vert_morph &m)
{
    for(size_t j=0;j<m.verts.size();++j)
    {
        pmd_morph_data::morph_vertex &v=m.verts[j];
        v.idx=reader.read<t>();
        v.pos.x=reader.read<float>();
        v.pos.y=reader.read<float>();
        v.pos.z=-reader.read<float>();
    }
}

template<typename t> void load_uv_morph(nya_memory::memory_reader &reader,pmx_loader::additional_data::uv_morph &m)
{
    for(size_t j=0;j<m.verts.size();++j)
    {
        add_data::morph_uv &v=m.verts[j];
        v.idx=reader.read<t>();
        v.tc.x=reader.read<float>();
        v.tc.y=-reader.read<float>();
        reader.skip(2*4); //ToDo
    }
}

inline void read_vector(nya_math::vec3 &out,nya_memory::memory_reader &reader)
{
    out.x=reader.read<float>(),out.y=reader.read<float>(),out.z=-reader.read<float>();
}

inline void read_angle(nya_math::vec3 &out,nya_memory::memory_reader &reader)
{
    out.x=-reader.read<float>(),out.y=-reader.read<float>(),out.z=reader.read<float>();
}

}

bool pmx_loader::load(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name)
{
    if(!data.get_size())
        return false;

    nya_memory::memory_reader reader(data.get_data(),data.get_size());
    if(!reader.test("PMX",3))
        return false;

    reader.skip(1);
    if(reader.read<float>()!=2.0f)
    {
        nya_log::log()<<"pmx load error: invalid version\n";
        return false;
    }

    const char header_size=reader.read<uint8_t>();
    if(header_size!=sizeof(pmx_header))
    {
        nya_log::log()<<"pmx load error: invalid header\n";
        return false;
    }
    
    const pmx_header header=reader.read<pmx_header>();
    
    for(int i=0;i<4;++i)
    {
        const int size=reader.read<int>();
        reader.skip(size);
    }
    
    const int vert_count=reader.read<int>();
    if(!vert_count)
    {
        nya_log::log()<<"pmx load error: no verts found\n";
        return false;
    }
    
    nya_memory::tmp_buffer_scoped verts_data(vert_count*sizeof(vert));
    vert *verts=(vert *)verts_data.get_data();
    for(int i=0;i<vert_count;++i)
    {
        vert &v=verts[i];
        read_vector(v.pos,reader);
        read_vector(v.normal,reader);

        v.tc.x=reader.read<float>();
        v.tc.y=1.0f-reader.read<float>();
        reader.skip(header.extended_uv*sizeof(float)*4);

        switch(reader.read<uint8_t>())
        {
            case 0:
                v.bone_idx[0]=(float)read_idx(reader,header.bone_idx_size);
                v.bone_weight[0]=1.0f;
                for(int j=1;j<4;++j)
                    v.bone_weight[j]=v.bone_idx[j]=0.0f;
                break;
                
            case 1:
                v.bone_idx[0]=(float)read_idx(reader,header.bone_idx_size);
                v.bone_idx[1]=(float)read_idx(reader,header.bone_idx_size);
                
                v.bone_weight[0]=reader.read<float>();
                v.bone_weight[1]=1.0f-v.bone_weight[0];
                
                for(int j=2;j<4;++j)
                    v.bone_weight[j]=v.bone_idx[j]=0.0f;
                break;
                
            case 2:
                for(int j=0;j<4;++j)
                    v.bone_idx[j]=(float)read_idx(reader,header.bone_idx_size);
                
                for(int j=0;j<4;++j)
                    v.bone_weight[j]=reader.read<float>();
                break;
                
            case 3:
                v.bone_idx[0]=(float)read_idx(reader,header.bone_idx_size);
                v.bone_idx[1]=(float)read_idx(reader,header.bone_idx_size);
                
                v.bone_weight[0]=reader.read<float>();
                v.bone_weight[1]=1.0f-v.bone_weight[0];
                
                for(int j=2;j<4;++j)
                    v.bone_weight[j]=v.bone_idx[j]=0.0f;

                reader.skip(sizeof(float)*3*3);
                break;

            default:
                nya_log::log()<<"pmx load error: invalid skining\n";
                return false;
        }
        
        float weight=0.0f;
        for(int k=0;k<4;++k)
        {
            float &w=v.bone_weight[k];
            if(w<0.001f)
            {
                w=0.0f;
                v.bone_idx[k]=0;
            }
            weight+=w;
        }
        weight=1.0f/weight;
        for(int k=0;k<4;++k)
            v.bone_weight[k]*=weight;

        v.morph_idx=0;
        reader.read<float>(); //edge
    }

    const int indices_count=reader.read<int>();
    const void *indices=reader.get_data();
    reader.skip(indices_count*header.index_size);

    const int textures_count=reader.read<int>();
    std::vector<std::string> tex_names(textures_count);
    for(int i=0;i<textures_count;++i)
        tex_names[i]=read_string(reader,header.text_encoding);

    const int mat_count=reader.read<int>();
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

    add_data *ad=new add_data;
    res.add_data=ad;

    ad->materials.resize(mat_count);
    
    nya_scene::texture white;

    for(int i=0,offset=0;i<mat_count;++i)
    {
        nya_scene::shared_mesh::group &g=res.groups[i];
        nya_scene::material &m=res.materials[i];
        additional_data::mat &am=ad->materials[i];
        m.load("mmd.txt");

        g.name=read_string(reader,header.text_encoding); //jp
        read_string(reader,header.text_encoding); //en
        m.set_name(g.name.c_str());

        const pmx_material_params &params=ad->materials[i].params=reader.read<pmx_material_params>();

        const unsigned char flag=reader.read<uint8_t>();
        const pmx_edge_params &edge=ad->materials[i].edge_params=reader.read<pmx_edge_params>();
        am.edge_group_idx=-1;
        am.backface_cull = !(flag & (1<<0));
        am.shadow_ground = flag & (1<<1);
        am.shadow_cast = flag & (1<<2);
        am.shadow_receive = flag & (1<<3);

        const int tex_idx=read_idx(reader,header.texture_idx_size);
        if(tex_idx>=0 && tex_idx<(int)tex_names.size())
            am.tex_diffuse=path+tex_names[tex_idx];

        const int sph_tex_idx=read_idx(reader,header.texture_idx_size);
        const int sph_mode=reader.read<int8_t>();
        if(sph_tex_idx>=0 && sph_tex_idx<(int)tex_names.size())
            am.tex_env=path+tex_names[sph_tex_idx];

        const unsigned char toon_flag=reader.read<uint8_t>();
        if(toon_flag==0)
        {
            const int toon_tex_idx=read_idx(reader,header.texture_idx_size);
            if(toon_tex_idx>=0 && toon_tex_idx<(int)tex_names.size())
                am.tex_toon=path+tex_names[toon_tex_idx];
        }
        else if(toon_flag==1)
        {
            const int toon_tex_idx=reader.read<int8_t>();
            char buf[255];
            sprintf(buf,"toon%02d.bmp",toon_tex_idx+1);
            if(nya_resources::get_resources_provider().has((path+buf).c_str()))
                am.tex_toon=path+buf;
            else
                am.tex_toon=buf;
        }
        else
        {
            nya_log::log()<<"pmx load error: unsupported toon flag\n";
            return false;
        }

        const int comment_len=reader.read<int>();
        reader.skip(comment_len);

        g.offset=offset;
        g.count=reader.read<int>();
        g.material_idx=i;
        offset+=g.count;

        if(m_load_textures)
        {
            nya_scene::texture tex;
            if(am.tex_diffuse.empty() || !tex.load(am.tex_diffuse.c_str()))
            {
                if(!white.get_width())
                    white.build("\xff\xff\xff\xff",1,1,nya_render::texture::color_bgra);
                m.set_texture("diffuse",white);
            }
            else
                m.set_texture("diffuse",tex);

            nya_scene::texture toon_tex;
            if(am.tex_toon.empty() || !toon_tex.load(am.tex_toon.c_str()))
            {
                if(!white.get_width())
                    white.build("\xff\xff\xff\xff",1,1,nya_render::texture::color_bgra);
                m.set_texture("toon",white);
            }
            else
            {
                nya_render::texture rtex=toon_tex.internal().get_shared_data()->tex;
                rtex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
                m.set_texture("toon",toon_tex);
            }

            nya_scene::texture env_tex;
            if(am.tex_env.empty() || !env_tex.load(am.tex_env.c_str()))
            {
                if(!white.get_width())
                    white.build("\xff\xff\xff\xff",1,1,nya_render::texture::color_bgra);
                m.set_texture("env",white);
            }
            else
            {
                nya_render::texture rtex=env_tex.internal().get_shared_data()->tex;
                rtex.set_wrap(nya_render::texture::wrap_clamp, nya_render::texture::wrap_clamp);
                m.set_texture("env",env_tex);
                if(sph_mode==2)
                    m.set_param("env param", 0.0f,1.0f);
                else if(sph_mode==1)
                    m.set_param("env param", 1.0f,0.0f);
            }
        }

        if(!(flag & (1<<0)))
        {
            for(int j=0;j<m.get_passes_count();++j)
            {
                if(!m.get_pass(j).get_state().cull_face)
                    m.get_pass(j).get_state().set_cull_face(true,nya_render::cull_face::ccw);
            }
        }

        m.set_param("amb k",params.ambient,1.0f);
        m.set_param("diff k",params.diffuse);
        m.set_param("spec k",params.specular,params.shininess);

        if(!(flag & (1<<4)))
            continue;

        ad->materials[i].edge_group_idx=(int)res.groups.size();

        res.groups.resize(res.groups.size()+1);
        nya_scene::shared_mesh::group &ge=res.groups.back();
        ge=res.groups[i];
        ge.name="edge";
        ge.material_idx=int(res.materials.size());
        res.materials.resize(res.materials.size()+1);
        nya_scene::material &me=res.materials.back();
        me.load("mmd_edge.txt");
        me.set_name(ge.name.c_str());
        me.set_param("edge offset",edge.width*0.03f,edge.width*0.03f,edge.width*0.03f,0.0f);
        me.set_param("edge color",edge.color);
    }

    typedef unsigned short ushort;

    const int bones_count=reader.read<int>();

    std::vector<pmx_bone> bones(bones_count);
    for(int i=0;i<bones_count;++i)
    {
        pmx_bone &b=bones[i];

        b.name=read_string(reader,header.text_encoding);
        skip_string(reader); //en name

        read_vector(b.pos,reader);
        b.idx=i;

        b.parent=read_idx(reader,header.bone_idx_size);
        b.order=reader.read<int>();

        const flag<uint16_t> f(reader.read<uint16_t>()); //ToDo
        if(f.c(0x0001))
            read_idx(reader,header.bone_idx_size);
        else
            reader.skip(sizeof(float)*3);

        b.bound.has_rot=f.c(0x0100);
        b.bound.has_pos=f.c(0x0200);
        if(b.bound.has_rot || b.bound.has_pos)
        {
            b.bound.src_idx=read_idx(reader,header.bone_idx_size);
            b.bound.k=reader.read<float>();
        }

        if(f.c(0x0400))
            reader.skip(sizeof(float)*3); //ToDo: fixed axis

        if(f.c(0x0800))
            reader.skip(sizeof(float)*3*2); //ToDo: local co-ordinate

        if(f.c(0x2000))
            reader.read<int>(); //ToDo: external parent deform

        b.ik.has=f.c(0x0020);
        if(b.ik.has)
        {
            b.ik.eff_idx=read_idx(reader,header.bone_idx_size);
            b.ik.count=reader.read<int>();
            b.ik.k=reader.read<float>();

            const int link_count=reader.read<int>();
            b.ik.links.resize(link_count);
            for(int j=0;j<link_count;++j)
            {
                pmx_bone::ik_link &l=b.ik.links[j];
                l.idx=read_idx(reader,header.bone_idx_size);
                l.has_limits=reader.read<int8_t>()>0;
                if(l.has_limits)
                {
                    read_angle(l.from,reader);
                    read_angle(l.to,reader);
                    std::swap(l.from.x,l.to.x);
                    std::swap(l.from.y,l.to.y);

                    //ToDo
                    l.from.z = -l.from.z;
                    l.to.z = -l.to.z;
                    std::swap(l.from.z,l.to.z);
                }
            }
        }
    }

    std::stable_sort(bones.begin(),bones.end(), compare_order);

    std::vector<int> old_bones(bones_count);
    for(int i=0;i<bones_count;++i)
        old_bones[bones[i].idx]=i;

    for(int i=0;i<int(bones.size());++i)
    {
        pmx_bone &b=bones[i];
        if(b.parent>=0)
            b.parent=old_bones[b.parent];
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

    for(int i=0;i<bones_count;++i)
        old_bones[bones[i].idx]=i;

    for(int i=0;i<bones_count;++i)
    {
        const pmx_bone &b=bones[i];

        if(res.skeleton.add_bone(b.name.c_str(),b.pos,nya_math::quat(),b.parent,true)!=i)
        {
            nya_log::log()<<"pmd load error: invalid bone["<<old_bones[i]<<"]: "<<b.name.c_str()<<"\n";
            return false;
        }
    }

    for(int i=0;i<bones_count;++i)
    {
        const pmx_bone &b=bones[i];

        if(b.ik.has)
        {
            const int ik=res.skeleton.add_ik(i,old_bones[b.ik.eff_idx],b.ik.count,b.ik.k,true);
            for(int j=0;j<int(b.ik.links.size());++j)
            {
                const pmx_bone::ik_link &l=b.ik.links[j];
                if(l.has_limits)
                    res.skeleton.add_ik_link(ik,old_bones[l.idx],l.from,l.to,true);
                else
                    res.skeleton.add_ik_link(ik,old_bones[l.idx],true);
            }
        }
        
        if((b.bound.has_pos || b.bound.has_rot) && b.bound.src_idx>=0 && b.bound.src_idx<bones_count)
          res.skeleton.add_bound(i, old_bones[b.bound.src_idx],b.bound.k,b.bound.has_pos,b.bound.has_rot,true);
    }

    if(bones_count>0)
    {
        for(int i=0;i<vert_count;++i)
        {
            vert &v=verts[i];
            for(int j=0;j<4;++j)
            {
                float &idx=v.bone_idx[j];
                if(idx>=0)
                    idx=(float)old_bones[(int)idx];
            }
        }
    }
    else
    {
        res.skeleton.add_bone("\xe3\x82\xbb\xe3\x83\xb3\xe3\x82\xbf\xe3\x83\xbc",nya_math::vec3(),nya_math::quat(),-1);
        old_bones.push_back(0);
    }

    const int morphs_count=reader.read<int>();
    ad->morphs.resize(morphs_count);

    for(int i=0;i<morphs_count;++i)
    {
        additional_data::morph &m=ad->morphs[i];

        m.name=read_string(reader,header.text_encoding);
        skip_string(reader); //en name

        m.kind=pmd_morph_data::morph_kind(reader.read<int8_t>());

        m.type=morph_type(reader.read<int8_t>());
        switch(m.type)
        {
            case morph_type_group:
            {
                m.idx=(int)ad->group_morphs.size();
                ad->group_morphs.resize(ad->group_morphs.size()+1);
                additional_data::group_morph &m=ad->group_morphs.back();

                const int size=reader.read<int>();
                m.morphs.resize(size);
                for(int j=0;j<size;++j)
                {
                    m.morphs[j].first=read_idx(reader,header.morph_idx_size);
                    m.morphs[j].second=reader.read<float>();
                }
            }
            break;

            case morph_type_vertex:
            {
                m.idx=(int)ad->vert_morphs.size();
                ad->vert_morphs.resize(ad->vert_morphs.size()+1);
                additional_data::vert_morph &m=ad->vert_morphs.back();

                const int size=reader.read<int>();
                m.verts.resize(size);

                switch(header.index_size)
                {
                    case 1: load_vertex_morph<uint8_t>(reader,m); break;
                    case 2: load_vertex_morph<uint16_t>(reader,m); break;
                    case 4: load_vertex_morph<uint32_t>(reader,m); break;
                }
            }
            break;

            case morph_type_bone:
            {
                m.idx=(int)ad->bone_morphs.size();
                ad->bone_morphs.resize(ad->bone_morphs.size()+1);
                additional_data::bone_morph &m=ad->bone_morphs.back();

                const int size=reader.read<int>();
                m.bones.resize(size);

                for(int j=0;j<size;++j)
                {
                    additional_data::morph_bone &b=m.bones[j];
                    b.idx=old_bones[read_idx(reader,header.bone_idx_size)];
                    read_vector(b.pos,reader);

                    b.rot.v.x=-reader.read<float>();
                    b.rot.v.y=-reader.read<float>();
                    b.rot.v.z=reader.read<float>();
                    b.rot.w=reader.read<float>();
                }
            }
            break;

            case morph_type_uv:
            case morph_type_add_uv1:
            case morph_type_add_uv2:
            case morph_type_add_uv3:
            case morph_type_add_uv4:
            {
                m.idx=(int)ad->uv_morphs.size();
                ad->uv_morphs.resize(ad->uv_morphs.size()+1);
                additional_data::uv_morph &m=ad->uv_morphs.back();

                const int size=reader.read<int>();
                m.verts.resize(size);

                switch(header.index_size)
                {
                    case 1: load_uv_morph<uint8_t>(reader,m); break;
                    case 2: load_uv_morph<uint16_t>(reader,m); break;
                    case 4: load_uv_morph<uint32_t>(reader,m); break;
                }
            }
            break;

            case morph_type_material:
            {
                m.idx=(int)ad->mat_morphs.size();
                ad->mat_morphs.resize(ad->mat_morphs.size()+1);
                additional_data::mat_morph &m=ad->mat_morphs.back();

                const int size=reader.read<int>();

                for(int j=0;j<size;++j)
                {
                    additional_data::morph_mat mm;
                    mm.mat_idx=read_idx(reader,header.material_idx_size);
                    mm.op=additional_data::morph_mat::op_type(reader.read<int8_t>());
                    mm.params=reader.read<pmx_material_params>();
                    mm.edge_params=reader.read<pmx_edge_params>();
                    mm.tex_color=reader.read<nya_math::vec4>();
                    mm.sp_tex_color=reader.read<nya_math::vec4>();
                    mm.toon_tex_color=reader.read<nya_math::vec4>();

                    if(mm.mat_idx>=0)
                        m.mats.push_back(mm);
                }
            }
            break;

            default:
                nya_log::log()<<"pmx load error: invalid morph type\n";
                return false;
        }
    }

    nya_scene::texture morphs_tex;
    int morphs_tex_w=1,morphs_tex_h=1;
    if (!ad->vert_morphs.empty() || !ad->uv_morphs.empty())
    {
        std::map<int,int> vmorph_remap;
        int last=1;
        for(int i=0;i<(int)ad->vert_morphs.size();++i)
        {
            for(auto &m: ad->vert_morphs[i].verts)
            {
                if(vmorph_remap.insert(std::make_pair(m.idx,last)).second)
                {
                    verts[m.idx].morph_idx=(float)last;
                    ++last;
                }
            }
        }
        for(int i=0;i<(int)ad->uv_morphs.size();++i)
        {
            for(auto &m: ad->uv_morphs[i].verts)
            {
                if(vmorph_remap.insert(std::make_pair(m.idx,last)).second)
                {
                    verts[m.idx].morph_idx=(float)last;
                    ++last;
                }
            }
        }
        ad->mvert_count=last;
        morphs_tex_w=512;
        const int morphs_tex_len=int((ad->vert_morphs.size()+ad->uv_morphs.size())*ad->mvert_count);
        while(morphs_tex_len > morphs_tex_w * morphs_tex_w)
            morphs_tex_w *= 2;
        morphs_tex_h=int((morphs_tex_len+morphs_tex_w-1)/morphs_tex_w); //ceil division
        nya_memory::tmp_buffer_scoped vert_morphs_buf(morphs_tex_w*morphs_tex_h*sizeof(nya_math::vec3));
        nya_math::vec3 *vert_morphs_data=(nya_math::vec3 *)vert_morphs_buf.get_data();
        memset(vert_morphs_data,0,vert_morphs_buf.get_size());
        ad->vert_tex_w=morphs_tex_w;
        ad->vert_tex_h=morphs_tex_h;

        for(int i=0;i<(int)ad->vert_morphs.size();++i)
        {
            nya_math::vec3 *mv=&vert_morphs_data[i*ad->mvert_count];
            for(auto &m: ad->vert_morphs[i].verts)
                mv[vmorph_remap[m.idx]]=m.pos;
        }
        for(int i=0;i<(int)ad->uv_morphs.size();++i)
        {
            nya_math::vec3 *mv=&vert_morphs_data[(i + ad->vert_morphs.size())*ad->mvert_count];
            for(auto &m: ad->uv_morphs[i].verts)
                mv[vmorph_remap[m.idx]].set(m.tc,0.0f);
        }
        morphs_tex.build(vert_morphs_data,morphs_tex_w,morphs_tex_h,nya_render::texture::color_rgb32f);
    }
    else
        morphs_tex.build(&nya_math::vec4::zero(),1,1,nya_render::texture::color_rgb32f);

    for(int i=0;i<(int)res.materials.size();++i)
    {
        res.materials[i].set_texture("morph",morphs_tex);
        res.materials[i].set_param("morphs count",0.0f,float(morphs_tex_w),float(morphs_tex_h));
    }

    const int frames_count=reader.read<int>();
    for(int i=0;i<frames_count;++i)
    {
        skip_string(reader); //name
        skip_string(reader); //en name

        reader.skip(1);//flag

        const int inner_count=reader.read<int>();
        for(int j=0;j<inner_count;++j)
        {
            char type=reader.read<int8_t>();
            read_idx(reader,type?header.morph_idx_size:header.bone_idx_size);
        }
    }

    std::vector<float> bone_radius_sq(bones_count,0.0f);
    nya_math::vec3 aabb_min,aabb_max;
    aabb_min=aabb_max=verts[0].pos;
    for(int i=0;i<vert_count;++i)
    {
        const vert &v=verts[i];
        aabb_min=nya_math::vec3::min(aabb_min,v.pos);
        aabb_max=nya_math::vec3::max(aabb_max,v.pos);

        const float min_weight = 1.0f / 4 - 0.0001f;
        for(int j=0;j<4;++j)
        {
            if(v.bone_weight[j]<min_weight)
                continue;

            const int idx=v.bone_idx[j];
            const float r_sq=(v.pos-bones[idx].pos).length_sq();
            if(r_sq>bone_radius_sq[idx])
                bone_radius_sq[idx]=r_sq;
        }
    }

    res.aabb=nya_math::aabb(aabb_min,aabb_max);
    res.aabb_bone_extends.reserve(bones_count);
    for(int i=0,j=0;i<bones_count;++i)
    {
        if(bone_radius_sq[i]<0.001f)
            continue;

        res.aabb_bone_extends.resize(j+1);
        res.aabb_bone_extends[j].idx=i;
        res.aabb_bone_extends[j].radius=sqrt(bone_radius_sq[i]);
        ++j;
    }

    const float tf=1.0f/bones_count;
    for(int i=0;i<vert_count;++i)
    {
        vert &v=verts[i];
        for(int j=0;j<4;++j)
            v.bone_idx[j]=(v.bone_idx[j]+0.5f)*tf;
    }

    res.vbo.set_vertex_data(&verts[0],sizeof(vert),(unsigned int)vert_count);
    verts_data.free();
    int offset=0;
    res.vbo.set_vertices(offset,4); offset+=sizeof(verts[0].pos); offset+=sizeof(float);
    res.vbo.set_normals(offset); offset+=sizeof(verts[0].normal);
    res.vbo.set_tc(0,offset,2); offset+=sizeof(verts[0].tc);
    res.vbo.set_tc(1,offset,4); offset+=sizeof(verts[0].bone_idx);
    res.vbo.set_tc(2,offset,4); //offset+=sizeof(verts[0].bone_weight);

    if(header.index_size==2)
    {
        const uint16_t *data=(uint16_t *)indices;
        nya_memory::tmp_buffer_scoped tmp_buf(indices_count*2);
        uint16_t *tmp=(uint16_t *)tmp_buf.get_data();
        for(uint32_t i=0;i<indices_count;i+=3)
        {
            tmp[i]=data[i];
            tmp[i+1]=data[i+2];
            tmp[i+2]=data[i+1];
        }
        res.vbo.set_index_data(tmp,nya_render::vbo::index2b,indices_count);
    }
    else if(header.index_size==4)
    {
        const uint32_t *data=(uint32_t *)indices;
        nya_memory::tmp_buffer_scoped tmp_buf(indices_count*4);
        uint32_t *tmp=(uint32_t *)tmp_buf.get_data();
        for(uint32_t i=0;i<indices_count;i+=3)
        {
            tmp[i]=data[i];
            tmp[i+1]=data[i+2];
            tmp[i+2]=data[i+1];
        }
        res.vbo.set_index_data(tmp,nya_render::vbo::index4b,indices_count);
    }
    else if(header.index_size==1)
    {
        const uint8_t *data=(uint8_t *)indices;
        nya_memory::tmp_buffer_scoped tmp_buf(indices_count*2);
        uint16_t *tmp=(uint16_t *)tmp_buf.get_data();
        for(int i=0;i<indices_count;++i)
        {
            tmp[i]=data[i];
            tmp[i+1]=data[i+2];
            tmp[i+2]=data[i+1];
        }
        res.vbo.set_index_data(tmp,nya_render::vbo::index2b,indices_count);
    }
    else
    {
        nya_log::log()<<"pmx load error: invalid index size\n";
        return false;
    }

    //physics

    const int rigid_bodies_count=reader.read<int>();
    ad->rigid_bodies.resize(rigid_bodies_count);
    for(int i=0;i<rigid_bodies_count;++i)
    {
        pmd_phys_data::rigid_body &rb=ad->rigid_bodies[i];

        rb.name=read_string(reader,header.text_encoding);
        skip_string(reader); //en name

        rb.bone=read_idx(reader,header.bone_idx_size);
        if(rb.bone>=0)
            rb.bone=old_bones[rb.bone];

        rb.collision_group=reader.read<uint8_t>();
        rb.collision_mask=reader.read<uint16_t>();
        rb.type=(pmd_phys_data::shape_type)reader.read<uint8_t>();
        rb.size=reader.read<nya_math::vec3>();
        read_vector(rb.pos,reader);
        rb.pos-=res.skeleton.get_bone_original_pos(rb.bone);
        read_angle(rb.rot,reader);
        rb.mass=reader.read<float>();
        rb.vel_attenuation=reader.read<float>();
        rb.rot_attenuation=reader.read<float>();
        rb.restriction=reader.read<float>();
        rb.friction=reader.read<float>();
        rb.mode=(pmd_phys_data::object_type)reader.read<uint8_t>();
    }

    const int joints_count=reader.read<int>();
    ad->joints.resize(joints_count);
    for(int i=0;i<joints_count;++i)
    {
        pmd_phys_data::joint &j=ad->joints[i];
        j.name=read_string(reader,header.text_encoding);
        skip_string(reader); //en name

        char type=reader.read<int8_t>();
        if(type>0)
        {
            nya_log::log()<<"pmx load warning: unsupported phys joint type\n";
            ad->joints.clear();
            ad->rigid_bodies.clear();
            break;
        }

        j.rigid_src=read_idx(reader,header.rigidbody_idx_size);
        j.rigid_dst=read_idx(reader,header.rigidbody_idx_size);
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
        j.rot_spring=reader.read<nya_math::vec3>();
    }

    return true;
}

const pmx_loader::additional_data *pmx_loader::get_additional_data(const nya_scene::mesh &m)
{
    if(!m.internal().get_shared_data().is_valid())
        return 0;

    nya_scene::shared_mesh::additional_data *d=m.internal().get_shared_data()->add_data;
    if(!d)
        return 0;

    const char *type=d->type();
    if(!type || strcmp(type,"pmx")!=0)
        return 0;

    return static_cast<pmx_loader::additional_data*>(static_cast<add_data*>(d));
}

bool pmx_loader::m_load_textures=true;
