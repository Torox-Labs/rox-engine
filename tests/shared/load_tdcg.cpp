//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "load_tdcg.h"
#include "scene/mesh.h"
#include "memory/memory_reader.h"
#include <zlib.h>
#include <sstream>

inline unsigned int swap_byte_order(unsigned int ui) { return (ui >> 24) | ((ui<<8) & 0x00FF0000) | ((ui>>8) & 0x0000FF00) | (ui << 24); }

std::string read_string(nya_memory::memory_reader &reader)
{
    std::string out;
    while(reader.get_remained())
    {
        const char c=reader.read<char>();
        if(!c)
            break;

        out.push_back(c);
    }

    return out;
}

struct shader_params
{
    std::map<std::string,std::string> strings;
    std::map<std::string,std::string> textures;
    std::map<std::string,float> floats;
    std::map<std::string,nya_math::vec4> vectors;

    bool parse_line(const std::string &s)
    {
        size_t type_from=0;
        while(s[type_from]<=' ') if(++type_from>=s.length()) return false;
        size_t type_to=type_from;
        while(s[type_to]>' ') if(++type_to>=s.length()) return false;

        size_t name_from=type_to+1;
        while(s[name_from]<=' ') if(++name_from>=s.length()) return false;
        size_t name_to=name_from;
        while(s[name_to]>' ' && s[name_to]!='=') if(++name_to>=s.length()) return false;

        const std::string name=s.substr(name_from,name_to-name_from);
        const std::string type=s.substr(type_from,type_to-type_from);

        const size_t eq_pos=s.find('=',name_to);
        if(eq_pos==std::string::npos)
            return false;

        if(type=="string" || type=="texture")
        {
            const size_t val_from=s.find('"',eq_pos+1);
            const size_t val_to=s.find('"',val_from+1);
            std::string value;
            if(val_from==std::string::npos || val_to==std::string::npos)
            {
                size_t val_from=eq_pos+1;
                while(s[val_from]<=' ') if(++val_from>=s.length()) return false;
                value=s.substr(val_from);
            }
            else
                value=s.substr(val_from+1,val_to-val_from);

            if(type=="string")
                strings[name]=value;
            else
                textures[name]=value;
        }
        else if(type=="float" || type=="float4")
        {
            const size_t val_from=s.find('[',eq_pos+1);
            const size_t val_to=s.find(']',val_from+1);
            if(val_from==std::string::npos || val_to==std::string::npos)
                return false;

            std::string value=s.substr(val_from+1,val_to-val_from);

            nya_math::vec4 v;
            std::replace(value.begin(),value.end(),',',' ');
            std::istringstream iss(value);
            if(type=="float4")
            {
                if(iss>>v.x) if(iss>>v.y) if(iss>>v.z) iss>>v.w;
                vectors[name]=v;
            }
            else
                iss>>floats[name];
        }
        else
            return false;

        return true;
    }
};

bool tdcg_loader::load_hardsave(nya_scene::shared_mesh &res,nya_scene::resource_data &data,const char* name)
{
    nya_memory::memory_reader reader(data.get_data(),data.get_size());

    const char *png_header="\x89PNG\x0d\x0a\x1a\x0a";
    if(!reader.test(png_header, sizeof(png_header)))
        return false;

    std::vector<vert> verts;
    std::map<std::string,nya_scene::texture> textures;

    typedef unsigned int uint;
    while(reader.get_remained())
    {
        const uint size=swap_byte_order(reader.read<uint>());
        const uint type=reader.read<uint>();
        if(memcmp(&type,"taOb",4)==0)
        {
            const size_t offset=reader.get_offset();

            const uint type=reader.read<uint>();
            reader.skip(8);
            const uint dst_len=reader.read<uint>();
            const uint src_len=reader.read<uint>();

            if(size<src_len || reader.get_remained()<src_len)
                return false;

            if(memcmp(&type,"FTSO",4)==0)
            {
                nya_memory::tmp_buffer_scoped buf(dst_len);
                uLong extracted_len=dst_len;
                if(uncompress((Byte *)buf.get_data(),&extracted_len,(Byte *)reader.get_data(),src_len)!=Z_OK)
                    return false;

                nya_memory::memory_reader reader(buf.get_data(),buf.get_size());
                if(!reader.test("TSO1", 4))
                    return false;

                const uint bones_count=reader.read<uint>();
                for(uint i=0;i<bones_count;++i)
                    read_string(reader); //ToDo

                const uint bone_mat_count=reader.read<uint>();
                if(bones_count!=bone_mat_count)
                    return false;

                for(uint i=0;i<bone_mat_count;++i)
                    reader.read<nya_math::mat4>(); //ToDo

                const uint tex_count=reader.read<uint>();
                for(uint i=0;i<tex_count;++i)
                {
                    const std::string name=read_string(reader);
                    const std::string file_name=read_string(reader);
                    const uint width=reader.read<uint>();
                    const uint height=reader.read<uint>();
                    const uint channels=reader.read<uint>();

                    nya_scene::shared_texture tex;

                    if(channels>0)
                    {
                        if(channels==4)
                            tex.tex.build_texture(reader.get_data(),width,height,nya_render::texture::color_rgba);
                        else
                            return false; //ToDo
                    }

                    reader.skip(width*height*channels);
                    textures[name].create(tex);
                }

                const uint shaders_count=reader.read<uint>();
                for(uint i=0;i<shaders_count;++i)
                {
                    const std::string name=read_string(reader);
                    const uint lines_count=reader.read<uint>();
                    for(uint j=0;j<lines_count;++j)
                        read_string(reader);
                }

                std::vector<shader_params> params;
                std::vector<std::string> material_names;

                const uint shader_params_count=reader.read<uint>();
                for(uint i=0;i<shader_params_count;++i)
                {
                    shader_params p;
                    const std::string name=read_string(reader);
                    const std::string file_name=read_string(reader);
                    const uint lines_count=reader.read<uint>();
                    for(uint j=0;j<lines_count;++j) if(!p.parse_line(read_string(reader))) return false;
                    params.push_back(p);
                    material_names.push_back(name);
                }

                const uint mesh_count=reader.read<uint>();
                for(uint i=0;i<mesh_count;++i)
                {
                    const std::string name=read_string(reader);
                    const nya_math::mat4 mat=reader.read<nya_math::mat4>();
                    reader.skip(4);
                    const uint groups_count=reader.read<uint>();
                    for(uint j=0;j<groups_count;++j)
                    {
                        nya_scene::shared_mesh::group g;
                        g.name=name;
                        g.offset=(uint)verts.size();
                        g.elem_type=nya_render::vbo::triangle_strip;

                        const uint shader_params_idx=reader.read<uint>();
                        const uint bone_indices_count=reader.read<uint>();
                        for(uint k=0;k<bone_indices_count;++k)
                            reader.read<uint>();

                        g.count=reader.read<uint>();
                        verts.resize(g.offset+g.count);
                        for(uint k=0;k<g.count;++k)
                        {
                            vert &v=verts[g.offset+k];
                            v.pos=mat*reader.read<nya_math::vec3>();
                            v.normal=(mat*nya_math::vec4(reader.read<nya_math::vec3>(),0.0f)).xyz().normalize();
                            v.tc=reader.read<nya_math::vec2>();
                            const uint skin_count=reader.read<uint>();
                            if(skin_count>4)
                            {
                                std::vector<std::pair<float,uint> > weights(skin_count);
                                for(uint l=0;l<skin_count;++l)
                                {
                                    weights[l].second=reader.read<uint>();
                                    weights[l].first=reader.read<float>();
                                }

                                std::sort(weights.rbegin(),weights.rend());
                                for(uint l=0;l<4;++l)
                                {
                                    v.bone_idx[l]=weights[l].second;
                                    v.bone_weight[l]=weights[l].first;
                                }
                            }
                            else
                            {
                                for(uint l=0;l<skin_count;++l)
                                {
                                    v.bone_idx[l]=reader.read<uint>();
                                    v.bone_weight[l]=reader.read<float>();
                                }

                                for(uint l=skin_count;l<4;++l)
                                    v.bone_weight[l]=0.0f;
                            }

                            float w=0.0f;
                            for(uint l=0;l<4;++l) w+=v.bone_weight[l];

                            const float eps=0.001f;
                            if(fabsf(1.0f-w)<eps && w>eps) for(uint l=0;l<4;++l) v.bone_weight[l]/=w;
                        }

                        g.material_idx=(uint)res.materials.size();
                        res.materials.resize(g.material_idx+1);
                        nya_scene::material &m=res.materials.back();
                        nya_scene::material::pass &p=m.get_pass(m.add_pass(nya_scene::material::default_pass));
                        p.set_shader(nya_scene::shader("tdcg.nsh"));

                        if(shader_params_idx>=(uint)params.size())
                            return false;

                        shader_params &sp=params[shader_params_idx];
                        for(std::map<std::string,std::string>::iterator it=sp.textures.begin();it!=sp.textures.end();++it)
                            m.set_texture(it->first.c_str(),textures[it->second]);

                        for(std::map<std::string,float>::iterator it=sp.floats.begin();it!=sp.floats.end();++it)
                            m.set_param(it->first.c_str(),it->second,0.0f,0.0f,0.0f);

                        for(std::map<std::string,nya_math::vec4>::iterator it=sp.vectors.begin();it!=sp.vectors.end();++it)
                            m.set_param(it->first.c_str(),it->second);

                        p.get_state().set_blend(true,nya_render::blend::src_alpha,nya_render::blend::inv_src_alpha);
                        p.get_state().set_cull_face(true);

                        res.groups.push_back(g);
                    }
                }
            }

            reader.seek(offset);
        }
        else if(memcmp(&type,"IEND",4)==0)
            break;

        if(!reader.skip(size))
            return false;

        reader.skip(4); //CRC
    }

    if(verts.empty())
        return false;

    res.vbo.set_vertex_data(&verts[0],(uint)sizeof(vert),(uint)verts.size());

#define off(st, m) uint((size_t)(&((st *)0)->m))
    res.vbo.set_vertices(off(vert,pos),3);
    res.vbo.set_normals(off(vert,normal));
    res.vbo.set_tc(0,off(vert,tc),2);
    res.vbo.set_tc(1,off(vert,bone_idx),4);
    res.vbo.set_tc(2,off(vert,bone_weight),4);

    return true;
}
