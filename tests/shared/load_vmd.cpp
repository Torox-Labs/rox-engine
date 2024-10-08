//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "load_vmd.h"
#include "scene/animation.h"
#include "memory/memory_reader.h"
#include "string_encoding.h"
#include <sstream>

const char *vmd_loader::ik_prefix="__disable_ik__";
const char *vmd_loader::display_curve="__hide_model__";

template<int length> struct comparer { bool operator()(const void* lhs,const void* rhs) const { return memcmp(lhs,rhs,length)<0; } };

bool vmd_loader::load(nya_scene::shared_animation &res,nya_scene::resource_data &data,const char* name)
{
    nya_memory::memory_reader reader(data.get_data(),data.get_size());

    if(!reader.test("Vocaloid Motion Data 0002",25))
        return false;

    reader.skip(5);
    reader.skip(20);//name

    typedef unsigned int uint;
    const int name_len=15;

    const uint frames_count=reader.read<uint>();
    if(!reader.check_remained(frames_count*(name_len+sizeof(uint)+sizeof(float)*7+16*4)))
        return false;

    const float time_scale=1000.0f/30;

    typedef std::map<const void *,int,comparer<name_len> > name_map;
    name_map names;

    struct
    {
        int idx;
        uint frame;

        nya_math::vec3 pos;
        nya_math::quat rot;

        const char *bezier;

        void get_bezier(int idx,nya_math::bezier &dst)
        {
            const char *src=bezier+idx*16;
            if(src[0]==src[4] && src[8]==src[12])
                return;

            const float c2f=1.0f/128.0f;
            dst=nya_math::bezier(src[0]*c2f,src[4]*c2f,src[8]*c2f,src[12]*c2f);
        }

    } bone_frame;

    for(uint i=0;i<frames_count;++i)
    {
        const name_map::iterator it=names.find(reader.get_data());
        if(it==names.end())
            names[reader.get_data()]=bone_frame.idx=res.anim.add_bone(utf8_from_shiftjis(reader.get_data(),name_len).c_str());
        else
            bone_frame.idx=it->second;
        reader.skip(name_len);

        bone_frame.frame=reader.read<uint>();
        bone_frame.pos.x=reader.read<float>();
        bone_frame.pos.y=reader.read<float>();
        bone_frame.pos.z=-reader.read<float>();

        bone_frame.rot.v.x=-reader.read<float>();
        bone_frame.rot.v.y=-reader.read<float>();
        bone_frame.rot.v.z=reader.read<float>();
        bone_frame.rot.w=reader.read<float>();

        bone_frame.bezier=(char *)reader.get_data();
        reader.skip(16*4);

        const uint time=uint(bone_frame.frame*time_scale);

        nya_render::animation::pos_interpolation pos_inter;
        bone_frame.get_bezier(0,pos_inter.x);
        bone_frame.get_bezier(1,pos_inter.y);
        bone_frame.get_bezier(2,pos_inter.z);
        res.anim.add_bone_pos_frame(bone_frame.idx,time,bone_frame.pos,pos_inter);

        nya_math::bezier rot_inter;
        bone_frame.get_bezier(3,rot_inter);
        res.anim.add_bone_rot_frame(bone_frame.idx,time,bone_frame.rot,rot_inter);
    }

    const uint morph_frames_count=reader.read<uint>();
    if(!reader.check_remained(morph_frames_count*(name_len+sizeof(uint)+sizeof(float))))
        return false;

    names.clear();

    for(uint i=0;i<morph_frames_count;++i)
    {
        int idx;
        const name_map::iterator it=names.find(reader.get_data());
        if(it==names.end())
            names[reader.get_data()]=idx=res.anim.add_curve(utf8_from_shiftjis(reader.get_data(),name_len).c_str());
        else
            idx=it->second;
        reader.skip(name_len);

        const uint frame=reader.read<uint>();
        const float value=reader.read<float>();
        res.anim.add_curve_frame(idx,uint(frame*time_scale),value);
    }

    const uint camera_frames_count=reader.read<uint>();
    if(!reader.skip(camera_frames_count*61))
        return true;

    const uint light_frames_count=reader.read<uint>();
    if(!reader.skip(light_frames_count*28))
        return true;

    const uint shadow_frames_count=reader.read<uint>();
    if(!reader.skip(shadow_frames_count*9))
        return true;

    names.clear();

    int display_idx=-1;
    const uint ik_frames_count=reader.read<uint>();
    for(int i=0;i<ik_frames_count;++i)
    {
        const uint time=uint(reader.read<uint>()*time_scale);
        const bool display=reader.read<char>()!=0;

        if(display_idx<0 && !display)
            display_idx=res.anim.add_curve(display_curve);
        if(display_idx>=0)
        {
            const float value=display?0.0f:1.0f;
            if(time>0)
            {
                const float prev=res.anim.get_curve(display_idx,time-1);
                if(prev!=value)
                    res.anim.add_curve_frame(display_idx,time-1,prev);
            }
            res.anim.add_curve_frame(display_idx,time,value);
        }

        const int count=reader.read<int>();
        for(int j=0;j<count;++j)
        {
            const void *name_buf=reader.get_data();
            if(!reader.skip(20))
                return true;

            const bool enabled=reader.read<char>()!=0;

            int idx;
            const name_map::iterator it=names.find(name_buf);
            if(it==names.end())
            {
                if(enabled)
                    continue;

                names[reader.get_data()]=idx=res.anim.add_curve((ik_prefix+(utf8_from_shiftjis(name_buf,20))).c_str());
            }
            else
                idx=it->second;

            const float value=enabled?0.0f:1.0f;
            if(time>0)
            {
                const float prev=res.anim.get_curve(idx,time-1);
                if(prev!=value)
                    res.anim.add_curve_frame(idx,time-1,prev);
            }
            res.anim.add_curve_frame(idx,time,value);
        }
    }

    return true;
}

bool vmd_loader::load_pose(nya_scene::shared_animation &res,nya_scene::resource_data &data,const char* name)
{
    const char sign[]="Vocaloid Pose Data file";
    if(data.get_size()<sizeof(sign) || memcmp(data.get_data(),sign,sizeof(sign)-1)!=0)
        return false;

    std::string text=utf8_from_shiftjis(data.get_data(),(unsigned int)data.get_size());
    size_t p=text.find(';'); //model name;
    if(p==std::string::npos)
        return false;

    p=text.find(';',p+1); //bones count;
    if(p==std::string::npos)
        return false;

    for(p=text.find('{',p);p!=std::string::npos;p=text.find('{',p))
    {
        ++p;
        size_t pe=text.find('\n',p);
        if(pe==std::string::npos)
            break;

        if(text[pe-1]=='\r')
            --pe;

        const std::string bone_name=text.substr(p,pe-p);

        nya_math::vec3 pos;

        p=pe;
        pe=text.find(';',p);
        if(pe==std::string::npos)
            break;

        std::replace(text.begin()+p,text.begin()+pe,',',' ');

        std::istringstream pss(text.substr(p,pe-p));
        pss>>pos.x,pss>>pos.y,pss>>pos.z;
        pos.z=-pos.z;

        nya_math::quat rot;

        p=text.find('\n',pe);
        if(p==std::string::npos)
            break;

        pe=text.find(';',p);
        if(pe==std::string::npos)
            break;

        std::replace(text.begin()+p,text.begin()+pe,',',' ');

        std::istringstream rss(text.substr(p,pe-p));
        rss>>rot.v.x,rss>>rot.v.y,rss>>rot.v.z,rss>>rot.w;
        rot.v.x=-rot.v.x;
        rot.v.y=-rot.v.y;

        const int bone_idx=res.anim.add_bone(bone_name.c_str());
        res.anim.add_bone_pos_frame(bone_idx,0,pos);
        res.anim.add_bone_rot_frame(bone_idx,0,rot);
    }

    return true;
}
