//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "load_xps.h"
#include "scene/mesh.h"
#include "memory/memory_reader.h"
#include "memory/invalid_object.h"
#include "resources/resources.h"

#include <sstream>

namespace
{
    struct color { unsigned char r,g,b,a; };
    struct face { unsigned int i0,i1,i2; };
    struct skining { unsigned short inds[4]; float weights[4]; };
    nya_scene::material::param_proxy light_dir=nya_scene::material::param_proxy(nya_scene::material::param(-0.58f,0.58f,0.57f,1.0f));
}

class text_reader
{
public:
    template<typename t> t read()
    {
        const std::string s=read_string();
        std::istringstream iss(s);
        t out;
        iss>>out;
        return out;
    }

    nya_math::vec2 read_tc()
    {
        const std::string s=read_string();
        std::istringstream iss(s);
        nya_math::vec2 v;
        if(iss>>v.x) iss>>v.y;
        v.y=1.0f-v.y;
        return v;
    }

    nya_math::vec3 read_vec3()
    {
        const std::string s=read_string();
        std::istringstream iss(s);
        nya_math::vec3 v;
        if(iss>>v.x) if(iss>>v.y) iss>>v.z;
        return v;
    }

    face read_face()
    {
        const std::string s=read_string();
        std::istringstream iss(s);
        face f; memset(&f,0,sizeof(f));
        if(iss>>f.i0) if(iss>>f.i1) iss>>f.i2;
        return f;
    }

    std::string read_string()
    {
        size_t i=m_offset;
        for(;i<m_size;++i)
        {
            const char c=*(m_text+i);
            if(!c || strchr("\n\r",c))
                break;
        }

        if(i<=m_offset)
            return std::string();

        const char *text=m_text+m_offset;
        const size_t len=i-m_offset;
        m_offset+=len;

        for(;m_offset<m_size;++m_offset)
        {
            const char c=*(m_text+m_offset);
            if(c && !strchr("\n\r",c))
                break;
        }

        return std::string(text,len);
    }

    skining read_skining()
    {
        const std::string str=read_string();
        const std::string str2=read_string();
        std::istringstream iss(str);
        std::istringstream iss2(str2);
        skining s; memset(&s,0,sizeof(s));
        if(iss>>s.inds[0]) if(iss>>s.inds[1]) if(iss>>s.inds[2]) iss>>s.inds[3];
        if(iss2>>s.weights[0]) if(iss2>>s.weights[1]) if(iss2>>s.weights[2]) iss2>>s.weights[3];
        return s;
    }

    color read_color()
    {
        const std::string s=read_string();
        std::istringstream iss(s);
        unsigned int c[4]={255,255,255,255};
        for(int i=0;i<4;++i) if(!(iss>>c[i])) break;
        color out;
        out.r=c[0],out.g=c[1],out.b=c[2],out.a=c[3];
        return out;
    }

    void skip(size_t) {}

    size_t get_remained() const
    {
        if(m_offset>=m_size)
            return 0;

        return m_size-m_offset;
    }

public:
    text_reader(const char *text,size_t size): m_text(size?text:0),m_size(text?size:0),m_offset(0)
    {
        if(size>3 && m_text && strncmp(m_text,"\xef\xbb\xbf",3)==0)
        {
            m_size-=3;
            m_text+=3;
        }
    }

private:
    const char *m_text;
    size_t m_size,m_offset;
};

class binary_reader: public nya_memory::memory_reader
{
public:
    template<typename t> t read(const char *str=0) { return memory_reader::read<t>(); }

    std::string read_string(const char *str=0)
    {
        unsigned int len=0;
        for(unsigned int i=0;get_remained();++i)
        {
            unsigned char len_byte=read<unsigned char>();
            len+=(len_byte & 0x7F) << (7*i);
            if(!(len_byte & 0x80))
                break;
        }

        if(len>get_remained())
            return "";

        std::string ret((const char *)get_data(),len);
        skip(len);
        return ret;
    }

    nya_math::vec2 read_tc() { nya_math::vec2 v; v.x=read<float>(); v.y=1.0f-read<float>(); return v; }
    nya_math::vec3 read_vec3() { return read<nya_math::vec3>(); }
    face read_face() { return read<face>(); }

    skining read_skining()
    {
        skining s;

        for(int i=0;i<4;++i)
            s.inds[i]=read<unsigned short>();

        for(int i=0;i<4;++i)
            s.weights[i]=read<float>();

        return s;
    }

    color read_color() { return read<color>(); }

    binary_reader(const void *data,size_t size): memory_reader(data,size) {}
};

struct render_group_props
{
    enum shading_mode
    {
        shading_no,
        shading_yes,
        shading_yes_no,
        shading_vertex
    };

    enum spec_mode
    {
        spec_no,
        spec_yes,
        spec_intencity
    };

    shading_mode shading;
    bool alpha;
    float spec_k;
    spec_mode spec;

    const char *semantics(unsigned int idx) const
    {
        if(idx>=(unsigned int)maps.size())
            return 0;

        return maps[idx].c_str();
    }

private:
    std::vector<std::string> maps;

public:
    render_group_props(): shading(shading_no),alpha(false),spec_k(0.0f) {}
    render_group_props(shading_mode shading,bool alpha,spec_mode spec_hl,bool bump_rep1,
                       bool bump_rep2,const char *tex,const char *tex2="",const char *tex3="",
                       const char *tex4="",const char *tex5="",const char *tex6="",const char *tex7="")
    {
        this->shading=shading;
        this->alpha=alpha;
        this->spec_k=0.0f;
        this->spec=spec_hl;

        //ToDo

        maps.push_back(tex);
        maps.push_back(tex2);
        maps.push_back(tex3);
        maps.push_back(tex4);
        maps.push_back(tex5);
        maps.push_back(tex6);
        maps.push_back(tex7);
    }

    render_group_props(const std::string &s)
    {
        *this=get(s);

        if(this->spec!=spec_no)
        {
            size_t p=s.find("_");
            if(p!=std::string::npos)
            {
                p=s.find("_",p+1);
                if(p!=std::string::npos)
                    spec_k=(float)atof(&s[p+1]);
            }
        }
    }

public:
    static const render_group_props &get(const std::string &s)
    {
        const static render_group_props xnl_props[]=
        {
        //1
            render_group_props(shading_yes,false,spec_yes,true,true,"diffuse","lightmap","bump","mask","bump1","bump2"),
            render_group_props(shading_yes,false,spec_yes,false,false,"diffuse","lightmap","bump"),
            render_group_props(shading_yes,false,spec_no,false,false,"diffuse","lightmap"),
            render_group_props(shading_yes,false,spec_yes,false,false,"diffuse","bump"),
            render_group_props(shading_yes,false,spec_no,false,false,"diffuse"),
        //6
            render_group_props(shading_yes,true,spec_yes,false,false,"diffuse","bump"),
            render_group_props(shading_yes,true,spec_no,false,false,"diffuse"),
            render_group_props(shading_yes,true,spec_yes,false,false,"diffuse","lightmap","bump"),
            render_group_props(shading_yes,true,spec_no,false,false,"diffuse","lightmap"),
            render_group_props(shading_no,false,spec_no,false,false,"diffuse"),
        //11
            render_group_props(shading_vertex,false,spec_yes,false,false,"diffuse","bump"),
            render_group_props(shading_vertex,true,spec_yes,false,false,"diffuse","bump"),
            render_group_props(shading_no,false,spec_no,false,false,"diffuse"),
            render_group_props(shading_no,false,spec_yes,false,false,"diffuse","bump"),
            render_group_props(shading_no,true,spec_yes,false,false,"diffuse","bump"),
        //16
            render_group_props(shading_yes,false,spec_no,false,false,"diffuse"),
            render_group_props(shading_yes,false,spec_no,false,false,"diffuse","lightmap"),
            render_group_props(shading_yes,false,spec_no,false,false,"diffuse"),
            render_group_props(shading_yes,false,spec_no,false,false,"diffuse","lightmap"),
            render_group_props(shading_yes,false,spec_yes,true,true,"diffuse","lightmap","bump","mask","bump1","bump2"),
        //21
            render_group_props(shading_no,true,spec_no,false,false,"diffuse"),
            render_group_props(shading_yes,false,spec_yes,true,true,"diffuse","lightmap","bump","mask","bump1","bump2","spec"),
            render_group_props(shading_yes,true,spec_yes,true,true,"diffuse","lightmap","bump","mask","bump1","bump2","spec"),
            render_group_props(shading_yes,false,spec_yes,false,false,"diffuse","lightmap","bump","spec"),
            render_group_props(shading_yes,true,spec_yes,false,false,"diffuse","lightmap","bump","spec"),
        //26
            render_group_props(shading_yes_no,false,spec_intencity,false,false,"diffuse","bump","env"),
            render_group_props(shading_yes_no,true,spec_intencity,false,false,"diffuse","bump","env"),
            render_group_props(shading_yes_no,false,spec_intencity,true,true,"diffuse","bump","mask","bump1","bump2","env"),
            render_group_props(shading_yes_no,true,spec_intencity,true,true,"diffuse","bump","mask","bump1","bump2","env"),
            render_group_props(shading_yes_no,false,spec_intencity,false,false,"diffuse","bump","emission"),
        //31
            render_group_props(shading_yes_no,true,spec_intencity,false,false,"diffuse","bump","emission"),
            render_group_props(shading_yes,false,spec_yes,false,false,"diffuse"),
            render_group_props(shading_yes,true,spec_yes,false,false,"diffuse"),
            render_group_props(),
            render_group_props(),
        //36
            render_group_props(shading_yes_no,false,spec_intencity,true,false,"diffuse","bump","emission"),
            render_group_props(shading_yes_no,true,spec_intencity,true,false,"diffuse","bump","emission"),
            render_group_props(shading_yes_no,false,spec_intencity,true,false,"diffuse","bump","spec","emission"),
            render_group_props(shading_yes_no,true,spec_intencity,true,false,"diffuse","bump","spec","emission"),
            render_group_props(shading_yes,false,spec_yes,true,false,"diffuse","bump","spec"),
        //41
            render_group_props(shading_yes,true,spec_yes,true,false,"diffuse","bump","spec"),
        };

        if(s.empty())
        {
            nya_log::log()<<"WARNING: invalid render group\n";
            return nya_memory::invalid_object<render_group_props>();
        }

        const unsigned int ridx=atoi(s.c_str())-1;
        if(ridx>=sizeof(xnl_props)/sizeof(xnl_props[0]))
        {
            nya_log::log()<<"WARNING: unknown render group "<<s<<"\n";
            return nya_memory::invalid_object<render_group_props>();
        }

        return xnl_props[ridx];
    }
};

template<typename vert_t,typename ind_t> void calculate_tangents(vert_t *verts,unsigned int vcount,ind_t *inds,unsigned int icount,int uvlayer=0)
{
    for(unsigned int i=0;i<icount;i+=3)
    {
        vert_t &v0=verts[inds[i]],&v1=verts[inds[i+1]],&v2=verts[inds[i+2]];

        const nya_math::vec3 &p0=v0.pos,&p1=v1.pos,&p2=v2.pos;
        const nya_math::vec2 &tc0=v0.tc(uvlayer),&tc1=v1.tc(uvlayer),&tc2=v2.tc(uvlayer);
        const nya_math::vec3 p10=p1-p0,p20=p2-p0;
        const nya_math::vec2 tc10=tc1-tc0,tc20=tc2-tc0;

        const nya_math::vec3 t=nya_math::vec3::normalize(tc20.y*p10 - tc10.y*p20);
        const nya_math::vec3 bt=nya_math::vec3::normalize(tc20.x*p10 - tc10.x*p20);

        v0.tangent+=t,v1.tangent+=t,v2.tangent+=t;
        v0.bitangent+=bt,v1.bitangent+=bt,v2.bitangent+=bt;
    }

    for(unsigned int i=0;i<vcount;++i)
    {
        vert_t &v=verts[i];
        v.tangent.normalize();
        v.bitangent.normalize();

        v.tangent=(v.tangent-v.normal*(v.normal.dot(v.tangent))).normalize();
        v.bitangent=(v.bitangent-v.normal*(v.normal.dot(v.bitangent))).normalize();
    }
}

template<typename reader_t>bool load_mesh(nya_scene::shared_mesh &res,reader_t &reader,const char* name,unsigned short version=1)
{
    if(version>2)
        return false;

    typedef unsigned int uint;
    uint bones_count=reader.template read<uint>();
    if(bones_count>32767) //sanity check
        return false; //not an error, probably different format

    for(uint i=0;i<bones_count;++i)
    {
        const std::string name=reader.read_string();
        const short parent=reader.template read<short>();
        const nya_math::vec3 pos=reader.read_vec3();
        res.skeleton.add_bone(name.c_str(),pos,nya_math::quat(),parent,true);
    }

    std::vector<xps_loader::vert> vertices;
    typedef unsigned short ushort;
    std::vector<uint> indices;

    std::string path(name);
    size_t p=path.rfind("/");
    if(p==std::string::npos)
        p=path.rfind("\\");
    if(p==std::string::npos)
        path.clear();
    else
        path.resize(p+1);

    uint groups_count=reader.template read<uint>();
    if(!groups_count || groups_count>4096) //sanity check
        return false; //not an error, probably different format

    const int max_bones_count=4096;

    for(uint i=0;i<groups_count;++i)
    {
        if(!reader.get_remained())
        {
            nya_log::log()<<"ERROR: eof on group "<<i<<" read, probably corrupted file\n";
            return false;
        }

        const std::string name=reader.read_string();
        const uint uvlayers=reader.template read<uint>();
        const uint tex_count=reader.template read<uint>();
        if(tex_count>32)// || uvlayers>4)
        {
            nya_log::log()<<"ERROR: unsupported count of textures "<<tex_count<<" or uv layers "<<uvlayers<<"\n";
            return false;
        }

        std::vector<std::string> tex_names(tex_count);
        std::vector<uint> tex_uvlayers(tex_count);
        for(uint j=0;j<tex_count;++j)
        {
            std::string name=reader.read_string();
            size_t p=name.rfind("/");
            if(p==std::string::npos)
                p=name.rfind("\\");
            if(p!=std::string::npos)
                name=name.substr(p+1);

            tex_names[j]=path+name;
            tex_uvlayers[j]=reader.template read<uint>();
        }

        const uint voffset=(uint)vertices.size();
        const uint vcount=reader.template read<uint>();
        if(vcount>reader.get_remained()) //rough check
        {
            nya_log::log()<<"ERROR: eof on group "<<i<<" vertices read, probably corrupted file\n";
            return false;
        }

        vertices.resize(voffset+vcount);
        for(uint j=voffset;j<voffset+vcount;++j)
        {
            xps_loader::vert &v=vertices[j];

            v.pos=reader.read_vec3();
            v.normal=reader.read_vec3();

            color c=reader.read_color();
            v.color.x=c.r,v.color.y=c.g,v.color.z=c.b,v.color.w=c.a;
            v.color/=255.0f;

            v.color.w=1.0f;

            v.tc01.xy()=reader.read_tc();

            for(uint j=1;j<uvlayers;++j)
            {
                reader.read_tc();

                if(version==1) reader.skip(4*4); //I dunno
            }

            if(version==1) reader.skip(4*4); //dunno

            if(bones_count)
            {
                skining s=reader.read_skining();

                float w=0;
                for(int k=0;k<4;++k)
                    w+=s.weights[k];

                if(w>0.0f)
                {
                    for(int k=0;k<4;++k)
                        s.weights[k]/=w;
                }
                else
                    s.weights[0]=1.0f;

                for(int k=0;k<4;++k)
                    v.bone_idx[k]=(float(s.inds[k])+0.5f)/max_bones_count, v.bone_weight[k]=s.weights[k];
            }
        }

        const uint ioffset=(uint)indices.size();
        const uint fcount=reader.template read<uint>();
        if(fcount>reader.get_remained()) //rough check
        {
            nya_log::log()<<"ERROR: eof on group "<<i<<" faces read, probably corrupted file\n";
            return false;
        }

        indices.resize(ioffset+fcount*3);
        for(uint j=0;j<fcount;++j)
        {
            uint *f=&indices[ioffset+j*3];

            const face ff=reader.read_face();
            f[0]=ff.i0+voffset;
            f[1]=ff.i2+voffset;
            f[2]=ff.i1+voffset;
        }

        res.groups.resize(i+1);
        nya_scene::shared_mesh::group &g=res.groups[i];
        g.name=name;
        g.offset=ioffset;
        g.count=fcount*3;
        g.material_idx=i;
        g.elem_type=nya_render::vbo::triangles;

        res.materials.resize(i+1);
        nya_scene::material &m=res.materials[i];

        const render_group_props rgp(name);
        if(rgp.alpha)
            m.load("xps_a.txt");
        else
            m.load("xps.txt");

        if(rgp.shading==rgp.shading_no)
            m.set_param("light k",1.0f,0.0f,0.0f,0.0f);
        else
        {
            m.set_param("light k",0.7f,0.5f,rgp.spec_k,0.0f);
            m.set_param("light dir",light_dir);
        }

        std::map<std::string,bool> has_semantics;
        for(int i=0;i<(int)tex_names.size();++i)
        {
            const char *semantics=rgp.semantics(i);
            if(!semantics)
                continue;

            nya_scene::texture tex;
            if(!tex.load(tex_names[i].c_str()))
                continue;

            m.set_texture(semantics,tex);
            has_semantics[semantics]=true;
        }

        const char *semantics[]={"diffuse","lightmap","bump","mask","bump1","bump2","spec","env","emission"};
        const char *white="\xff\xff\xff\xff",*black="\x00\x00\x00\x00",*normal="\x7f\x7f\xff\xff";
        const char *default_textures[]={white,white,normal,white,normal,normal,white,black,black};

        for(int i=0;i<sizeof(semantics)/sizeof(semantics[0]);++i)
        {
            if(has_semantics.find(semantics[i])!=has_semantics.end())
                continue;

            nya_scene::texture tex;
            tex.build(default_textures[i],1,1,nya_render::texture::color_rgba);
            m.set_texture(semantics[i],tex);
        }
    }

    if(vertices.empty() || indices.empty())
        return false;

    calculate_tangents(&vertices[0],(uint)vertices.size(),&indices[0],(uint)indices.size());

    res.vbo.set_vertex_data(&vertices[0],(uint)sizeof(vertices[0]),(uint)vertices.size());

    #define off(st, m) uint((size_t)(&((st *)0)->m))

    res.vbo.set_colors(off(xps_loader::vert,color),4);
    res.vbo.set_normals(off(xps_loader::vert,normal));
    res.vbo.set_tc(4,off(xps_loader::vert,tangent),3);
    res.vbo.set_tc(5,off(xps_loader::vert,bitangent),3);
    res.vbo.set_tc(0,off(xps_loader::vert,tc01),4);
    res.vbo.set_tc(1,off(xps_loader::vert,tc23),4);
    res.vbo.set_tc(6,off(xps_loader::vert,bone_idx),4);
    res.vbo.set_tc(7,off(xps_loader::vert,bone_weight),4);

    if(vertices.size()<65535)
    {
        std::vector<unsigned short> indices2b(indices.size());
        for(size_t i=0;i<indices.size();++i)
            indices2b[i]=indices[i];
        res.vbo.set_index_data(&indices2b[0],nya_render::vbo::index2b,(uint)indices2b.size());
    }
    else
        res.vbo.set_index_data(&indices[0],nya_render::vbo::index4b,(uint)indices.size());

    return true;
}

bool xps_loader::load_mesh(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name)
{
    binary_reader r(data.get_data(),data.get_size());
    if(r.read<unsigned int>()==323232)
    {
        const unsigned short version=r.read<unsigned short>();
        r.read<unsigned short>();
        r.read_string();
        const unsigned int skip_count=r.read<unsigned int>();
        r.read_string();
        r.read_string();
        r.read_string();
        r.skip(skip_count*4);

        return ::load_mesh(res,r,name,version);
    }

    r.seek(0);

    return ::load_mesh(res,r,name);
}

bool xps_loader::load_mesh_ascii(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name)
{
    text_reader r((const char *)data.get_data(),data.get_size());
    return ::load_mesh(res,r,name);
}

bool xps_loader::load_pose(nya_scene::shared_animation &res,nya_scene::resource_data &data,const char* name)
{
    text_reader r((const char *)data.get_data(),data.get_size());
    while(r.get_remained())
    {
        std::string s=r.read_string();
        size_t name_end=s.find(':');
        if(name_end==std::string::npos)
            break;

        std::string name=s.substr(0,name_end);
        s=s.substr(name_end+1);
        std::istringstream iss(s);
        nya_math::vec3 angle,pos,scale;
        if(iss>>angle.x) if(iss>>angle.y) if(iss>>angle.z)
            if(iss>>pos.x) if(iss>>pos.y) if(iss>>pos.z)
                if(iss>>scale.x) if(iss>>scale.y) iss>>scale.z;

        angle*=nya_math::constants::pi/180.0f;
        const int bone=res.anim.add_bone(name.c_str());
        res.anim.add_bone_pos_frame(bone,0,pos);
        res.anim.add_bone_rot_frame(bone,0,nya_math::quat(angle.x,angle.y,angle.z));
        //ToDo: scale
    }

    return true;
}

void xps_loader::set_light_dir(const nya_math::vec3 &dir) { light_dir->set(dir,0.0f); }
