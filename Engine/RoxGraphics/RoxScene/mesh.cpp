//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "camera.h"
#include "RoxMath/RoxConstants.h"
#include "RoxMemory/RoxInvalidObject.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxMemory/RoxTmpBuffers.h"
#include "RoxFormats/RoxStringConvert.h"
#include "RoxFormats/RoxMesh.h"
#include "RoxFormats/RoxMesh.h"
#include "RoxRender/RoxRender.h"
#include "scene.h"
#include "shader.h"
#include <cstdint>

#include "material.h"
#include "mesh.h"

namespace RoxScene
{

namespace
{

    material::pass &material_default_pass(material &m)
{
    int idx=m.get_pass_idx(material::default_pass);
    if(idx<0)
        idx=m.add_pass(material::default_pass);

    return m.get_pass(idx);
}

bool frustum_cull_enabled=true;

}

bool mesh::load_nms_mesh_section(shared_mesh &res,const void *data,size_t size,int version)
{
    RoxFormats::nms_mesh_chunk c;
    if(!c.read_header(data,size,version))
    {
        log()<<"nms load warning: invalid RoxFormats::RMesh chunk\n";
        return false;
    }

    res.aabb=RoxMath::Aabb(c.aabb_min,c.aabb_max);

    for(size_t i=0;i<c.elements.size();++i)
    {
        const RoxFormats::nms_mesh_chunk::element &e=c.elements[i];
        const RoxRender::RoxVbo::VERTEX_ATRIB_TYPE type=RoxRender::RoxVbo::VERTEX_ATRIB_TYPE(e.data_type);
        switch(e.type)
        {
            case RoxFormats::nms_mesh_chunk::pos: res.vbo.setVertices(e.offset,e.dimension,type); break;
            case RoxFormats::nms_mesh_chunk::normal: res.vbo.setNormals(e.offset,type); break;
            case RoxFormats::nms_mesh_chunk::color: res.vbo.setColors(e.offset,e.dimension,type); break;
            default: res.vbo.setTc(e.type-RoxFormats::nms_mesh_chunk::tc0,e.offset,e.dimension,type); break;
        };
    }

    res.vbo.setVertexData(c.vertices_data,c.vertex_stride,c.verts_count);

    switch(c.index_size)
    {
        case 0: break; //to indices
        case 2: res.vbo.setIndexData(c.indices_data,RoxRender::RoxVbo::INDEX_2D,c.indices_count); break;
        case 4: res.vbo.setIndexData(c.indices_data,RoxRender::RoxVbo::INDEX_4D,c.indices_count); break;
        default: log()<<"nms load warning: invalid index size\n"; return false;
    }

    for(size_t i=0;i<c.lods.size();++i)
    {
        res.groups.resize(c.lods[i].groups.size());
        for(size_t j=0;j<res.groups.size();++j)
        {
            const RoxFormats::nms_mesh_chunk::group &from=c.lods[i].groups[j];
            shared_mesh::group &to=res.groups[j];

            to.name=from.name;

            to.aabb=RoxMath::Aabb(from.aabb_min,from.aabb_max);

            to.material_idx=from.material_idx;
            to.offset=from.offset;
            to.count=from.count;

            to.elem_type=RoxRender::RoxVbo::ELEMENT_TYPE(from.element_type);
        }

        break; //ToDo: load all lods
    }

    return true;
}

bool mesh::load_nms_skeleton_section(shared_mesh &res,const void *data,size_t size,int version)
{
    RoxFormats::nms_skeleton_chunk c;
    if(!c.read(data,size,version))
    {
        log()<<"nms load warning: invalid skeleton chunk\n";
        return false;
    }

    for(size_t i=0;i<c.bones.size();++i)
    {
        RoxFormats::nms_skeleton_chunk::bone &b=c.bones[i];
        res.skeleton.addBone(b.name.c_str(),b.pos,b.rot,b.parent);
    }

    return true;
}

bool mesh::load_nms_material_section(shared_mesh &res,const void *data,size_t size,int version)
{
    RoxFormats::nms_material_chunk c;
    if(!c.read(data,size,version))
    {
        log()<<"nms load warning: invalid materials chunk\n";
        return false;
    }

    size_t mat_idx_off=res.materials.size();
    res.materials.resize(mat_idx_off+c.materials.size());
    for(size_t i=0;i<c.materials.size();++i)
    {
        const RoxFormats::nms_material_chunk::material_info &from=c.materials[i];
        material &to=res.materials[i+mat_idx_off];

        for(size_t j=0;j<from.strings.size();++j)
        {
            const std::string &name=from.strings[j].name;
            const std::string &value=from.strings[j].value;

            if(name=="nya_material")
            {
                to.load(value.c_str());
            }
            else if(name=="nya_shader")
            {
                RoxShader sh;
                sh.load(value.c_str());
                material_default_pass(to).set_shader(sh);
            }
            else if(name=="nya_blend")
            {
                RoxRender::State &st=material_default_pass(to).get_state();
                st.blend=RoxFormats::blendModeFromString(value.c_str(),st.blend_src,st.blend_dst);
            }
            else if(name=="nya_cull")
            {
                RoxRender::State &st=material_default_pass(to).get_state();
                st.cull_face=RoxFormats::cullFaceFromString(value.c_str(),st.cull_order);
            }
            else if(name=="nya_zwrite")
                material_default_pass(to).get_state().zwrite=RoxFormats::boolFromString(value.c_str());
        }

        for(size_t j=0;j<from.textures.size();++j)
        {
            texture tex;
            tex.load(from.textures[j].filename.c_str());
            to.set_texture(from.textures[j].semantics.c_str(),tex);
        }

        for(size_t j=0;j<from.vectors.size();++j)
        {
            const int param_idx=to.get_param_idx(from.vectors[j].name.c_str());
            if(param_idx<0)
                continue;

            to.set_param(param_idx,from.vectors[j].value);
        }

        to.set_name(from.name.c_str());
    }

    return true;
}

bool mesh::load_nms_general_section(shared_mesh &res,const void *data,size_t size,int version)
{
    RoxFormats::nms_general_chunk c;
    if(!c.read(data,size,version))
    {
        log()<<"nms load warning: invalid general chunk\n";
        return false;
    }

    size_t idx_off=res.misc.size();
    res.misc.resize(idx_off+c.objects.size());
    for(size_t i=0;i<c.objects.size();++i)
    {
        const RoxFormats::nms_general_chunk::object &from=c.objects[i];
        shared_mesh::misc_info &to=res.misc[i+idx_off];
        to.name=from.name;
        to.type=from.type;

        to.string_params.resize(from.strings.size());
        for(size_t j=0;j<from.strings.size();++j)
        {
            to.string_params[j].first=from.strings[j].name;
            to.string_params[j].second=from.strings[j].value;
        }

        to.vec4_params.resize(from.vectors.size());
        for(size_t j=0;j<from.vectors.size();++j)
        {
            to.vec4_params[j].first=from.vectors[j].name;
            to.vec4_params[j].second=from.vectors[j].value;
        }
    }

    return true;
}

bool mesh::load_nms(shared_mesh &res,resource_data &data,const char* name)
{
    if(!data.getSize() || data.getSize()<8 || memcmp(data.getData(),"nya RoxFormats::RMesh",8)!=0)
        return false;

    RoxFormats::RMesh m;
    if(!m.readChunksInfo(data.getData(),data.getSize()))
    {
        log()<<"nms load error: invalid nms\n";
        return false;
    }

    if(m.version!=1 && m.version!=2)
    {
        log()<<"nms load error: unsupported version: "<<m.version<<"\n";
        return false;
    }

    for(size_t i=0;i<m.chunks.size();++i)
    {
        const RoxFormats::RMesh::ChunkInfo c=m.chunks[i];
        switch(c.type)
        {
        case RoxFormats::RMesh::MESH_DATA: if(!load_nms_mesh_section(res,c.data,c.size,m.version)) return false; break;
            case RoxFormats::RMesh::SKELETON: if(!load_nms_skeleton_section(res,c.data,c.size,m.version)) return false; break;
            case RoxFormats::RMesh::MATERIALS: if(!load_nms_material_section(res,c.data,c.size,m.version)) return false; break;
            case RoxFormats::RMesh::GENERAL: if(!load_nms_general_section(res,c.data,c.size,m.version)) return false; break;
            //default: log()<<"nms load warning: unknown chunk type\n"; //not an error
        };
    }

    return true;
}

bool mesh_internal::init_from_shared()
{
    if(!m_shared.isValid())
        return false;

    m_replaced_materials.clear();
    m_replaced_materials_idx.clear();
    m_anims.clear();

    m_skeleton=m_shared->skeleton;
    m_bone_controls.clear();

    for(int i=0;i<int(m_shared->materials.size());++i)
        m_shared->materials[i].internal().skeleton_changed(&m_skeleton);

    m_recalc_aabb=true;
    m_has_aabb=m_shared->aabb.delta.lengthSq()>0.0001f;

    m_groups.resize(m_shared->groups.size());
    for(int i=0;i<(int)m_groups.size();++i)
        m_groups[i].has_aabb=m_shared->groups[i].aabb.delta.lengthSq()>0.0001f;

    return true;
}

bool mesh::load(const char *name)
{
    mesh_internal::default_load_function(load_nms);

    if(!m_internal.load(name))
        return false;

    return m_internal.init_from_shared();
}

void mesh::create(const shared_mesh &res)
{
    m_internal.create(res);
    m_internal.init_from_shared();
}

void mesh::unload()
{
    m_internal.unload();

    for(size_t i=0;i<internal().m_replaced_materials.size();++i)
        m_internal.m_replaced_materials[i].unload();

    m_internal.m_replaced_materials.clear();
    m_internal.m_replaced_materials_idx.clear();
    m_internal.m_anims.clear();
    m_internal.m_skeleton=RoxRender::RoxSkeleton();
    m_internal.m_aabb=RoxMath::Aabb();
    m_internal.m_groups.clear();
}

int mesh_internal::get_materials_count() const
{
    if(!m_shared.isValid())
        return 0;

    return int(m_shared->materials.size()+m_replaced_materials.size());
}

const material &mesh_internal::mat(int idx) const
{
    const int shared_mat_count=(int)m_shared->materials.size();
    if(idx<shared_mat_count)
        return m_shared->materials[idx];

    return m_replaced_materials[idx-shared_mat_count];
}

int mesh_internal::get_mat_idx(int group_idx) const
{
    if(!m_shared.isValid())
        return -1;

    if(group_idx<0)
        return -1;

    if(group_idx<(int)m_replaced_materials_idx.size())
        return m_replaced_materials_idx[group_idx];

    if(group_idx>=(int)m_shared->groups.size())
        return -1;

    const shared_mesh::group &g=m_shared->groups[group_idx];
    if(g.material_idx>=m_shared->materials.size())
        return -1;

    return (int)g.material_idx;
}

void mesh_internal::draw_group(int idx, const char *pass_name) const
{
    if(!m_shared.isValid())
        return;

    if(idx<0 || idx>=(int)m_shared->groups.size())
        return;

    update_skeleton();

    int mat_idx=get_mat_idx(idx);
    if(mat_idx<0)
    {
        RoxLogger::warning()<<"invalid material for group'"<<idx<<"in RoxFormats::RMesh"<<get_name()<<"\n";
        return;
    }

    const shared_mesh::group &g=m_shared->groups[idx];

    transform::set(m_transform);
    shader_internal::set_skeleton(&m_skeleton);

    const material &m=mat(mat_idx);
    m.internal().set(pass_name);
    m_shared->vbo.bind();
    m_shared->vbo.draw(g.offset,g.count,g.elem_type);
    m_shared->vbo.unbind();
    m.internal().unset();

    shader_internal::set_skeleton(0);
}

void mesh::draw(const char *pass_name) const
{
    if(!pass_name)
        return;

    if(internal().m_has_aabb && frustum_cull_enabled && !get_camera().get_frustum().testIntersect(get_aabb()))
        return;

    for(int i=0;i<get_groups_count();++i)
        draw_group(i,pass_name);
}

void mesh::draw_group(int idx,const char *pass_name) const
{
    if(!pass_name)
        return;

    int mat_idx=internal().get_mat_idx(idx);
    if(mat_idx<0)
        return;

    if(internal().mat(mat_idx).get_pass_idx(pass_name)<0)
        return;

    if(frustum_cull_enabled)
    {
        internal().update_aabb_transform();
        if(internal().m_groups[idx].has_aabb)
        {
            if(!get_camera().get_frustum().testIntersect(internal().m_groups[idx].aabb))
                return;
        }
        else if(internal().m_has_aabb && !get_camera().get_frustum().testIntersect(get_aabb()))
            return;
    }

    internal().draw_group(idx,pass_name);
}

bool mesh::has_pass(const char *pass_name) const
{
    if(!pass_name)
        return false;

    for(int i=0;i<get_groups_count();++i)
    {
        int mat_idx=internal().get_mat_idx(i);
        if(mat_idx<0)
            continue;

        if(internal().mat(mat_idx).get_pass_idx(pass_name)>=0)
            return true;
    }

    return false;
}

int mesh::get_groups_count() const
{
    if(!internal().m_shared.isValid())
        return 0;

    return int(internal().m_shared->groups.size());
}

const char *mesh::get_group_name(int group_idx) const
{
    if(group_idx<0 || group_idx>=get_groups_count())
        return 0;

    return internal().m_shared->groups[group_idx].name.c_str();
}

const material &mesh::get_material(int group_idx) const
{
    int mat_idx=internal().get_mat_idx(group_idx);
    if(mat_idx<0)
        return RoxMemory::invalidObject<material>();

    return internal().mat(mat_idx);
}

material &mesh::modify_material(int idx)
{
    if(!internal().m_shared.isValid())
        return RoxMemory::invalidObject<material>();

    if(idx<0 || idx>=(int)internal().m_shared->groups.size())
        return RoxMemory::invalidObject<material>();

    int shared_mat_count=(int)internal().m_shared->materials.size();

    if(internal().m_replaced_materials_idx.empty())
    {
        m_internal.m_replaced_materials_idx.resize(internal().m_shared->groups.size());
        for(int i=0;i<(int)internal().m_shared->groups.size();++i)
            m_internal.m_replaced_materials_idx[i]=internal().m_shared->groups[i].material_idx;

        //preallocate because modify_material shouldn't ruin get_material
        m_internal.m_replaced_materials.resize(m_internal.m_replaced_materials_idx.size());
    }
    else
    {
        int replaced_idx=internal().m_replaced_materials_idx[idx];
        if(replaced_idx>=shared_mat_count)
            return m_internal.m_replaced_materials[replaced_idx-shared_mat_count];
    }

    m_internal.m_replaced_materials_idx[idx]=shared_mat_count+idx;
    if((int)internal().m_shared->groups[idx].material_idx>=shared_mat_count)
        return m_internal.m_replaced_materials[idx];

    m_internal.m_replaced_materials[idx]=internal().m_shared->materials[internal().m_shared->groups[idx].material_idx];
    return m_internal.m_replaced_materials[idx];
}

bool mesh::set_material(int idx,const material &mat)
{
    if(idx<0 || idx>=internal().get_materials_count())
        return false;

    modify_material(idx)=mat;
    return true;
}

void mesh::set_anim(const animation_proxy & anim,int layer,bool lerp)
{
    if(!internal().m_shared.isValid() || layer<0)
        return;

    int idx= -1;
    for(int i=0;i<int(internal().m_anims.size());++i)
    {
        if(m_internal.m_anims[i].layer==layer)
        {
            idx=i;
            break;
        }
    }

    if(!anim.isValid())
    {
        if(idx>=0)
            m_internal.m_anims.erase(m_internal.m_anims.begin()+idx);

        return;
    }

    if(idx<0)
    {
        idx=int(internal().m_anims.size());
        m_internal.m_anims.resize(internal().m_anims.size()+1);
    }

    mesh_internal::applied_anim &a=m_internal.m_anims[idx];
    a.layer=layer;
    a.anim=anim;
    a.time=0;
    a.version=0;
    a.lerp=lerp;
    a.bones_map.clear();
}

void mesh_internal::anim_set_time(applied_anim &a,float t)
{
    if(!a.anim.isValid())
    {
        a.time=0.0f;
        return;
    }

    const unsigned int anim_len=a.anim->m_range_to-a.anim->m_range_from;
    if(!anim_len)
    {
        a.time=0.0f;
        return;
    }

    a.time=t;

    const float anim_len_f=float(anim_len);
    if(!a.anim->get_loop())
    {
        if(a.time>anim_len_f)
            a.time=anim_len_f;

        if(a.time<0.0f)
            a.time=0.0f;

        return;
    }

    while(a.time>anim_len_f)
        a.time-=anim_len_f;

    while(a.time<0.0f)
        a.time+=anim_len_f;
}

void mesh_internal::anim_update_mapping(applied_anim &a)
{
    a.bones_map.clear();

    if(!a.anim.isValid() || !a.anim->m_shared.isValid())
        return;

    const RoxRender::RoxAnimation &ra=a.anim->m_shared->anim;
    a.bones_map.resize(get_bones_count(),-1);

    if(a.anim->m_mask.isValid())
    {
        for(int j=0;j<get_bones_count();++j)
        {
            const char *bone_name=m_skeleton.getBoneName(j);
            if(a.anim->m_mask->data.find(bone_name)!=a.anim->m_mask->data.end())
                a.bones_map[j]=ra.getBoneIdx(bone_name);
        }
    }
    else
    {
        for(int j=0;j<get_bones_count();++j)
            a.bones_map[j]=ra.getBoneIdx(m_skeleton.getBoneName(j));
    }
}

void mesh_internal::update_aabb_transform() const
{
    if(!m_shared.isValid())
        return;

    if(!m_recalc_aabb)
        return;

    if(m_shared->aabb_bone_extends.empty() || (m_bone_controls.empty() && m_anims.empty()))
    {
        m_aabb=m_transform.transform_aabb(m_shared->aabb);
        for(int i=0;i<(int)m_groups.size();++i)
        {
            if(!m_groups[i].has_aabb)
                continue;

            m_groups[i].aabb=m_transform.transform_aabb(m_shared->groups[i].aabb);
        }
    }
    else
    {
        const RoxRender::RoxSkeleton &sk=get_skeleton();
        const RoxMath::Vector3 scale=RoxMath::Vector3::abs(m_transform.get_scale());
        const shared_mesh::aabb_bone_extend &e=m_shared->aabb_bone_extends[0];
        RoxMath::Vector3 pos=m_transform.transform_vec(sk.getBonePos(e.idx));
        RoxMath::Vector3 aabb_min=pos-scale*e.radius;
        RoxMath::Vector3 aabb_max=pos+scale*e.radius;
        for(int i=1,size=(int)m_shared->aabb_bone_extends.size();i<size;++i)
        {
            const shared_mesh::aabb_bone_extend &e=m_shared->aabb_bone_extends[i];
            pos=m_transform.transform_vec(sk.getBonePos(e.idx));
            aabb_min=RoxMath::Vector3::min(aabb_min,pos-scale*e.radius);
            aabb_max=RoxMath::Vector3::max(aabb_max,pos+scale*e.radius);
        }
        m_aabb=RoxMath::Aabb(aabb_min,aabb_max);
    }

    m_recalc_aabb=false;
}

const animation_proxy & mesh::get_anim(int layer) const
{
    for(int i=0;i<int(internal().m_anims.size());++i)
    {
        if(internal().m_anims[i].layer==layer)
            return internal().m_anims[i].anim;
    }

    return RoxMemory::invalidObject<animation_proxy>();
}

void mesh::set_anim_time(unsigned int time,int layer)
{
    for(int i=0;i<int(internal().m_anims.size());++i)
    {
        if(internal().m_anims[i].layer==layer)
        {
            m_internal.anim_set_time(m_internal.m_anims[i],float(time));
            return;
        }
    }
}

unsigned int mesh::get_anim_time(int layer) const
{
    for(int i=0;i<int(internal().m_anims.size());++i)
    {
        if(internal().m_anims[i].layer==layer)
            return (unsigned int)(internal().m_anims[i].time);
    }

    return 0;
}

bool mesh_internal::is_anim_finished(int layer) const
{
    for(int i=0;i<int(m_anims.size());++i)
    {
        if(m_anims[i].layer==layer)
        {
            if(!m_anims[i].anim.isValid())
                return true;

            if(m_anims[i].anim->get_loop())
                return false;

            const float eps = 0.001f;
            if(m_anims[i].anim->m_speed>=0.0f)
                return fabs(m_anims[i].anim->m_range_to-m_anims[i].anim->m_range_from-m_anims[i].time)<eps;
            else
                return fabs(m_anims[i].time)<eps;
        }
    }

    return true;
}

bool mesh::is_anim_finished(int layer) const { return internal().is_anim_finished(layer); }

RoxMath::Vector3 mesh::get_bone_pos(int bone_idx,bool local,bool ignore_animations) const
{
    const RoxMath::Vector3 pos=ignore_animations?internal().get_skeleton().getBoneOriginalPos(bone_idx):
                                               internal().get_skeleton().getBonePos(bone_idx);
    if(local)
        return pos;

    return internal().get_transform().transform_vec( pos );
}

RoxMath::Quaternion mesh::get_bone_rot(int bone_idx,bool local) const
{
    const RoxMath::Quaternion rot=internal().get_skeleton().getBoneRot(bone_idx);
    if(local)
        return rot;

    return internal().get_transform().transform_quat( rot );
}

void mesh::set_bone_pos(int bone_idx,const RoxMath::Vector3 &pos,bool additive)
{
    if(bone_idx<0 || bone_idx>=get_bones_count())
        return;

    mesh_internal::bone_control &b=m_internal.m_bone_controls[bone_idx];
    b.pos=pos;
    b.pos_ctrl=additive?mesh_internal::bone_additive:mesh_internal::bone_override;
    internal().need_update_skeleton=true;
}

void mesh::set_bone_rot(int bone_idx,const RoxMath::Quaternion &rot,bool additive)
{
    if(bone_idx<0 || bone_idx>=get_bones_count())
        return;

    mesh_internal::bone_control &b=m_internal.m_bone_controls[bone_idx];
    b.rot=rot;
    b.rot_ctrl=additive?mesh_internal::bone_additive:mesh_internal::bone_override;
    internal().need_update_skeleton=true;
}

void mesh::update(unsigned int dt) { m_internal.update(dt); }

void mesh_internal::update(unsigned int dt)
{
    if(!m_shared.isValid())
        return;

    if(m_anims.empty() && m_bone_controls.empty())
        return;

    for(int i=0;i<(int)m_anims.size();++i)
    {
        applied_anim &a=m_anims[i];
        anim_set_time(a,a.time+dt*a.anim->m_speed);
        if(a.version!=a.anim->m_version)
        {
            anim_update_mapping(a);
            a.version=a.anim->m_version;
        }

        const float eps=0.0001f;
        a.full_weight=(fabsf(1.0f-a.anim->m_weight)<eps);
    }

    need_update_skeleton=true;
    m_recalc_aabb=true;
}

void mesh_internal::update_skeleton() const
{
    if(!need_update_skeleton)
        return;

    need_update_skeleton=false;
    m_recalc_aabb=true;

    for(int i=0;i<get_bones_count();++i)
    {
        RoxMath::Vector3 pos;
        RoxMath::Quaternion rot;

        float slerp_weight=0.0f;
        bool has_slerp=false;

        for(int j=0;j<(int)m_anims.size();++j)
        {
            const applied_anim &a=m_anims[j];
            if(i>=(int)a.bones_map.size() || a.bones_map[i]<0)
                continue;

            const unsigned int time=(unsigned int)a.time+a.anim->m_range_from;
            RoxMath::Vector3 bone_pos=a.anim->m_shared->anim.getBonePos(a.bones_map[i],time,a.anim->get_loop());
            RoxMath::Quaternion bone_rot=a.anim->m_shared->anim.getBoneRot(a.bones_map[i],time,a.anim->get_loop());

            if(a.lerp)
            {
                slerp_weight += a.anim->m_weight;
                has_slerp=true;

                if(j==0)
                {
                    pos=bone_pos;
                    rot=bone_rot;
                }
                else if (slerp_weight > 0.0001f && a.anim->m_weight > 0.0001f)
                {
                    const float k=a.anim->m_weight/slerp_weight;
                    pos=RoxMath::Vector3::lerp(pos,bone_pos,k);
                    rot=RoxMath::Quaternion::slerp(rot,bone_rot,k);
                }
            }
            else
            {
                if(!a.full_weight)
                    bone_pos*=a.anim->m_weight,bone_rot.applyWeight(a.anim->m_weight);

                if(j==0)
                    pos=bone_pos,rot=bone_rot;
                else
                    pos+=bone_pos,rot=rot*bone_rot;
            }
        }

        if(has_slerp && slerp_weight < 0.9999f)
        {
            pos*=slerp_weight;
            rot=RoxMath::Quaternion::slerp(RoxMath::Quaternion(),rot,slerp_weight);
        }

        bone_control_map::const_iterator it=m_bone_controls.find(i);
        if(it!=m_bone_controls.end())
        {
            const bone_control &b=it->second;

            switch(b.pos_ctrl)
            {
                case bone_override: pos=b.pos; break;
                case bone_additive: pos+=b.pos; break;
                case bone_free: break;
            }

            switch(b.rot_ctrl)
            {
                case bone_override: rot=b.rot; break;
                case bone_additive: rot=rot*b.rot; break;
                case bone_free: break;
            }
        }

        m_skeleton.setBoneTransform(i,pos,rot);
    }

    m_skeleton.update();

    const int mat_count=get_materials_count();
    for(int i=0;i<mat_count;++i)
        mat(i).internal().skeleton_changed(&m_skeleton);
}

const RoxMath::Aabb &mesh::get_aabb() const
{
    internal().update_aabb_transform();
    return internal().m_aabb;
}

void mesh::set_rot(RoxMath::AngleDeg yaw,RoxMath::AngleDeg pitch,RoxMath::AngleDeg roll)
{
    m_internal.m_transform.set_rot(yaw,pitch,roll); m_internal.m_recalc_aabb=true;
}

bool mesh::is_frustrum_cull_enabled() { return frustum_cull_enabled; }
void mesh::set_frustum_cull(bool enable) { frustum_cull_enabled=enable; }

}
