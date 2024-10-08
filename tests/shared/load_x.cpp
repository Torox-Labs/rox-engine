//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//ToDo: template parsing, FrameTransformMatrix

#include "load_x.h"
#include "load_pmd.h"
#include "scene/mesh.h"

namespace
{

class text_reader
{
public:
    std::string get_word()
    {
        skip_whitespace();
        std::string s;
        if(m_size>0 && *m_data=='"')
        {
            ++m_data,--m_size;
            while(m_size>0 && *m_data!='"') s.push_back(*m_data++),--m_size;
            if(m_size>0)
                ++m_data,--m_size;
        }
        else
            while(m_size>0 && (isalnum(*m_data) || *m_data=='_')) s.push_back(*m_data++),--m_size;
        return s;
    }

    int get_int()
    {
        std::string s;
        while(m_size>0 && !strchr(",;",*m_data)) s.push_back(*m_data++),--m_size;
        if(m_size)
            ++m_data,--m_size;
        return atoi(s.c_str());
    }

    float get_float()
    {
        std::string s;
        while(m_size>0 && !strchr(",;",*m_data)) s.push_back(*m_data++),--m_size;
        if(m_size>0)
            ++m_data,--m_size;
        return (float)atof(s.c_str());
    }

    bool next_element()
    {
        if(strchr(",;",*m_data))
        {
            ++m_data,--m_size;
            return true;
        }

        return false;
    }

    bool start_section()
    {
        skip_whitespace();
        if(!m_size || *m_data!='{')
            return false;

        ++m_data,--m_size;
        skip_whitespace();
        return true;
    }

    bool end_section()
    {
        skip_whitespace();
        if(!m_size || *m_data!='}')
            return false;

        ++m_data,--m_size;
        skip_whitespace();
        return true;
    }

    bool skip_section(int count=0)
    {
        while(m_size>0)
        {
            if(*m_data=='{')
                ++count;
            else if(*m_data=='}')
            {
                if(--count==0)
                {
                    ++m_data,--m_size;
                    return true;
                }
            }

            ++m_data,--m_size;
        }

        return false;
    }

    bool not_empty() const { return m_size>0; }

public:
    text_reader(const char *data,size_t size): m_data(data),m_size(size) {}

private:
    void skip_whitespace()
    {
        while(m_size>0)
        {
            if(isspace(*m_data))
            {
                ++m_data,--m_size;
                continue;
            }

            if(m_data[0]=='/' && m_data[1]=='/') //comment
            {
                while(m_size>0 && *m_data!='\n')
                    ++m_data,--m_size;
                continue;
            }

            break;
        }
    }

private:
    const char *m_data;
    size_t m_size;
};

struct xmaterial
{
    std::vector<int> finds;
    std::string tex_diff;
    nya_math::vec4 diff_k;
    float spec_pow;
    nya_math::vec3 spec_k;
    nya_math::vec3 em_k;

    xmaterial(): spec_pow(0) {}
};

bool load_mesh(std::string path,text_reader &tr,std::vector<x_loader::vert> &verts,nya_scene::shared_mesh &res)
{
    if(!tr.start_section())
        return false;

    std::vector<nya_math::vec3> positions(tr.get_int());
    for(int i=0;i<(int)positions.size();++i)
    {
        positions[i].x=tr.get_float();
        positions[i].y=tr.get_float();
        positions[i].z=-tr.get_float();
        tr.next_element();
    }

    const int fcount=tr.get_int();
    int vcount=0;
    std::vector<std::vector<int> > faces(fcount);
    for(int i=0;i<fcount;++i)
    {
        std::vector<int> &vinds=faces[i];

        const int c=tr.get_int();
        const int v0=tr.get_int();
        int v1=tr.get_int();
        for(int j=0;j<c-2;++j)
        {
            vinds.push_back(v0);
            vinds.push_back(v1);
            vinds.push_back(tr.get_int());
            v1=vinds.back();
        }
        vcount+=(int)vinds.size();
        tr.next_element();
    }

    std::vector<nya_math::vec2> tcs;
    std::vector<xmaterial> mats;

    while(tr.not_empty() && !tr.end_section())
    {
        std::string s=tr.get_word();
        //printf("\t%s\n", s.c_str());

        if(s=="MeshTextureCoords")
        {
            if(!tr.start_section())
                return false;

            tcs.resize(tr.get_int());
            for(int i=0;i<(int)tcs.size();++i)
            {
                tcs[i].x=tr.get_float();
                tcs[i].y=1.0f-tr.get_float();
                tr.next_element();
            }

            if(!tr.end_section())
                return false;
        }
        else if(s=="MeshMaterialList")
        {
            if(!tr.start_section())
                return false;

            mats.resize(tr.get_int());
            const int count=tr.get_int();
            for(int i=0;i<count;++i)
                mats[tr.get_int()].finds.push_back(i);
            tr.next_element(); //optional

            int idx=0;
            while(tr.not_empty() && !tr.end_section())
            {
                s=tr.get_word();
                //printf("\t\t%s\n",s.c_str());

                if(s=="Material")
                {
                    xmaterial &m=mats[idx++];
                    tr.start_section();

                    m.diff_k.x=tr.get_float();
                    m.diff_k.y=tr.get_float();
                    m.diff_k.z=tr.get_float();
                    m.diff_k.w=tr.get_float();
                    tr.next_element();
                    m.spec_pow=tr.get_float();
                    m.spec_k.x=tr.get_float();
                    m.spec_k.y=tr.get_float();
                    m.spec_k.z=tr.get_float();
                    tr.next_element();
                    m.em_k.x=tr.get_float();
                    m.em_k.y=tr.get_float();
                    m.em_k.z=tr.get_float();
                    tr.next_element();

                    while(tr.not_empty() && !tr.end_section())
                    {
                        s=tr.get_word();
                        //printf("\t\t\t%s\n",s.c_str());

                        if(s=="TextureFilename")
                        {
                            tr.start_section();
                            m.tex_diff=tr.get_word();
                            tr.next_element();
                            tr.end_section();
                        }
                        else if(!tr.skip_section())
                            return false;
                    }
                }
                else if(!tr.skip_section())
                    return false;
            }
        }
        else if(!tr.skip_section())
            return false;
    }

    const bool has_tc=tcs.size()==positions.size();

    const int voff=(int)verts.size();
    verts.resize(voff+vcount);
    const int goff=(int)res.groups.size();
    res.groups.resize(goff+mats.size());
    res.materials.resize(goff+mats.size());
    for(int i=0,v=voff;i<(int)mats.size();++i)
    {
        const xmaterial &mf=mats[i];

        nya_scene::shared_mesh::group &g=res.groups[goff+i];
        g.material_idx=goff+i;
        g.offset=v;

        nya_scene::material &mt=res.materials[goff+i];

        mt.get_default_pass().set_shader("x.nsh");

        nya_scene::texture base,spa,sph;
        pmd_loader::load_textures(path,mf.tex_diff,base,spa,sph);
        mt.set_texture("diffuse",base);
        mt.set_texture("env add",spa);
        mt.set_texture("env mult",sph);
        mt.set_param("diff k",mf.diff_k);
        mt.set_param("amb k",mf.em_k,1.0f);
        mt.set_param("spec k",mf.spec_k,mf.spec_pow);

        for(int j=0;j<(int)mf.finds.size();++j)
        {
            const std::vector<int> &f=faces[mf.finds[j]];
            for(int j=0;j<(int)f.size();++j,++v)
            {
                const int vind=f[j];
                verts[v].pos=positions[vind];
                if(has_tc)
                    verts[v].tc=tcs[vind];
            }
        }

        g.count=v-g.offset;
    }

    return true;
}

bool load_frame(std::string path,text_reader &tr,std::vector<x_loader::vert> &verts,nya_scene::shared_mesh &res)
{
    std::string s=tr.get_word();

    if(!tr.start_section())
        return false;

    while(tr.not_empty() && !tr.end_section())
    {
        s=tr.get_word();

        //printf("\t%s\n",s.c_str());

        if(s=="Frame")
        {
            if(!load_frame(path,tr,verts,res))
                return false;
        }
        else if(s=="Mesh")
        {
            if(!load_mesh(path,tr,verts,res))
                return false;
        }
        else if(!tr.skip_section())
            return false;
    }
    
    return true;
}

}

bool x_loader::load_text(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name)
{
    const char x_header[]="xof ";
    const size_t x_header_size=strlen(x_header);
    if(data.get_size()<x_header_size || memcmp(data.get_data(),x_header,x_header_size)!=0)
        return false;

    text_reader tr((char *)data.get_data()+x_header_size,data.get_size()-x_header_size);

    const std::string type=tr.get_word();
    if(type.substr(4)!="txt")
        return false;

    const std::string version=tr.get_word();

    std::string path(name);
    size_t p=path.rfind("/");
    if(p==std::string::npos)
        p=path.rfind("\\");
    if(p==std::string::npos)
        path.clear();
    else
        path.resize(p+1);

    std::vector<vert> verts;

    while(tr.not_empty())
    {
        std::string s=tr.get_word();
        if(s=="template")
        {
            s=tr.get_word();
            //printf("<template> %s\n", s.c_str());
            if(!tr.skip_section())
                return false;
            continue;
        }

        if(s.empty())
            break;

        //printf("%s\n", s.c_str());

        if(s=="Frame")
        {
            if(!load_frame(path,tr,verts,res))
                return false;
        }
        else if(s=="Mesh")
        {
            if(!load_mesh(path,tr,verts,res))
                return false;
        }
        else if(!tr.skip_section())
            return false;
    }

    if(verts.empty())
        return false;

    res.vbo.set_vertex_data(&verts[0],(int)sizeof(vert),(int)verts.size());

#define off(st, m) int((size_t)(&((st *)0)->m))
    res.vbo.set_vertices(off(vert,pos),3);
    res.vbo.set_normals(off(vert,normal));
    res.vbo.set_tc(0,off(vert,tc),2);

    return true;
}
