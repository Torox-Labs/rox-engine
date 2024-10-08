//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "character.h"
#include "attributes.h"
#include "tsb_anim.h"
#include "string.h"
#include "config.h"
#include "render/render.h"

void character::set_attrib(const char *key,const char *value,int num)
{
    if(!key || num>=part::max_models_per_part)
    {
        nya_log::log()<<"Unable to set character attribute: invalid input params\n";
        return;
    }

    if(strcmp(key,"COORDINATE")==0)
    {
        reset_attrib();

        attribute *a=get_attribute_manager().get(key,value);
        if(!a)
        {
            nya_log::log()<<"Invalid attribute "<<value<<" of type "<<key<<"\n";
            return;
        }

        set_attrib("BODY",a->get_value("COORD_00"));
        set_attrib("EYE",a->get_value("COORD_01"));
        set_attrib("UNDER",a->get_value("COORD_02"),0);
        set_attrib("UNDER",a->get_value("COORD_03"),1);
        set_attrib("SOCKS",a->get_value("COORD_04"));
        set_attrib("COSTUME",a->get_value("COORD_05"),0);
        set_attrib("COSTUME",a->get_value("COORD_06"),1);
        set_attrib("HEAD",a->get_value("COORD_07"));
        set_attrib("FACE",a->get_value("COORD_08"));
        set_attrib("NECK",a->get_value("COORD_09"));
        set_attrib("ARM",a->get_value("COORD_10"));
        set_attrib("SHOES",a->get_value("COORD_11"));
        set_attrib("HAIR",a->get_value("COORD_12"));

        return;
    }

    part_id id=get_part_id(key);
    if(id==invalid_part)
    {
        nya_log::log()<<"Unable to set character attribute: unknown key\n";
        return;
    }

    if(id==under)
    {
        if(num==0) set_attrib(key,value,2);
        if(num==1) set_attrib(key,value,3);

        if(num==3 || num<0)
            m_under_count=2;
    }

    part &p=m_parts[id];

    if(!value || strcasecmp(value,"nil")==0)
    {
        if(id==body)
            value="imo_bodyA_00";
        else if(id==eye)
            value="imo_eye_00";
        else if(id==hair)
            value="imo_hairA_00";
        else
        {
            if(num<0)
                p.free_models();
            else if(num<part::max_models_per_part)
            {
                p.subparts[num].model.free();
                p.subparts[num].value="nil";
            }
            return;
        }
    }

    std::string value_str(value);
    const char last=value_str[value_str.length()-1];
    if(last>='A' && last<='D')
    {
        value_str.resize(value_str.length()-1);
        //num=last-'A';
    }

    attribute *a=get_attribute_manager().get(key,value);
    if(!a)
    {
        value=value_str.c_str();
        a=get_attribute_manager().get(key,value);
        if(!a)
        {
            //nya_log::log()<<"Invalid attribute "<<value<<" of type "<<key<<"\n";
            value="nil";
        }
        //else
        //    num=last-'A';
    }

    if(!value[0] || strcasecmp(value,"nil")==0)
    {
        if(num<0)
            p.free_models();
        else if(num<part::max_models_per_part)
        {
            p.subparts[num].model.free();
            p.subparts[num].value="nil";
        }
        return;
    }

    bool should_outline=!get_outline_ignore().should_ignore(value);

    if(num<0)
    {
        int max_models=part::max_models_per_part;
        for(int i=0;i<max_models;++i)
        {
            //nya_log::log()<<p.subparts[i].value.c_str()<<"->"<<value<<"\n";

            if(p.subparts[i].value==value)
                continue;

            p.subparts[i].model.free();
            p.subparts[i].value.assign(value);
            p.subparts[i].outline=should_outline;

            char key[7]="FILE_";
            key[5]=i+'0';
            const char *model_name=a->get_value(key);
            if(!model_name || strcmp(model_name,"nil")==0)
            {
                p.subparts[i].value.assign("nil");
                continue;
            }

            //crunch
            if(id==under && strstr(model_name,"HC."))
                m_under_count=1;
 
            //nya_log::log()<<"model "<<i<<" "<<model_name<<"\n";

            model_ref ref=get_shared_models().access(model_name);
            if(!ref.is_valid())
            {
                nya_log::log()<<"Unable to set character attribute::Invalid model ref\n";
                p.subparts[i].value.assign("nil");
                continue;
            }

            p.subparts[i].model=ref;
        }
    }
    else if(p.subparts[num].value!=value)
    {
        //nya_log::log()<<p.subparts[num].value.c_str()<<"=>"<<value<<"\n";

        char key[7]="FILE_";
        key[5]=num+'0';
        model_ref &ref=p.subparts[num].model;
        ref.free();
        
        p.subparts[num].value.assign(value);
        p.subparts[num].outline=should_outline;

        const char *model_name=a->get_value(key);
        if(!model_name||strcmp(model_name,"nil")==0)
            return;
        
        //crunch
        if(id==under && strstr(model_name,"HC."))
            m_under_count=1;

        ref=get_shared_models().access(model_name);
        if(!ref.is_valid())
            nya_log::log()<<"Unable to set character attribute::Invalid model ref\n";
    }

    if(id==body && p.subparts[0].model.is_valid())
    {
        m_body_blend_group_idx=-1;

        set_anim(m_anim_name.c_str());
        m_body_group_count=p.subparts[0].model->get_groups_count();
        for(int i=0;i<m_body_group_count;++i)
        {
            const char *name=p.subparts[0].model->get_group_name(i);
            if(!name)
                continue;

            if(strstr(name,"matu"))
            {
                m_body_blend_group_idx=i;
                break;
            }
        }
    }
}

const char *character::get_attrib(const char *key,int num)
{
    if(!key || num>=2)
        return 0;

    part_id id=get_part_id(key);
    if(id==invalid_part)
    {
        if(strcmp(key,"COORDINATE")==0)
        {
            //ToDo
            return 0;
        }

        return 0;
    }

    if(num<0)
        num=0;

    return m_parts[id].subparts[num].value.c_str();
}

void character::copy_attrib(const character &from)
{
    set_attrib("BODY",from.m_parts[body].subparts[0].value.c_str());
    set_attrib("EYE",from.m_parts[eye].subparts[0].value.c_str());
    set_attrib("UNDER",from.m_parts[under].subparts[0].value.c_str(),0);
    set_attrib("UNDER",from.m_parts[under].subparts[1].value.c_str(),1);
    set_attrib("SOCKS",from.m_parts[socks].subparts[0].value.c_str());
    set_attrib("COSTUME",from.m_parts[costume].subparts[0].value.c_str(),0);
    set_attrib("COSTUME",from.m_parts[costume].subparts[1].value.c_str(),1);
    set_attrib("HEAD",from.m_parts[head].subparts[0].value.c_str());
    set_attrib("FACE",from.m_parts[face].subparts[0].value.c_str());
    set_attrib("NECK",from.m_parts[neck].subparts[0].value.c_str());
    set_attrib("ARM",from.m_parts[arm].subparts[0].value.c_str());
    set_attrib("SHOES",from.m_parts[shoes].subparts[0].value.c_str());
    set_attrib("HAIR",from.m_parts[hair].subparts[0].value.c_str());

    for(int i=0;i<max_parts;++i)
        for(int j=0;j<2;++j)
            m_parts[i].subparts[j].opacity=from.m_parts[i].subparts[j].opacity;

    m_under_state[0]=from.m_under_state[0];
    m_under_state[1]=from.m_under_state[1];   
    m_under_count=from.m_under_count;
}

void character::reset_attrib()
{
    for(int i=0;i<max_parts;++i)
        m_parts[i].free_models();
}

void character::set_anim(const char *anim_name)
{
    if(!anim_name)
        return;

    m_anim_name.assign(anim_name);

    //nya_log::log()<<"Set anim: "<<anim_name<<"\n";

    model_ref m=m_parts[body].subparts[0].model;
    if(!m.is_valid())
    {
        nya_log::log()<<"Unable to set character anim: invalid base part model ref\n";
        return;
    }

    std::string name(anim_name);
    name.append(".tsb");

    anim_ref a = get_shared_anims().access(name.c_str());
    if(!a.is_valid())
    {
        nya_log::log()<<"Unable to set character anim: invalid animation\n";
        return;
    }

    m_anim.apply_anim(*m.const_get(),*a.const_get());

    a.free();
}

void character::set_part_opacity(const char *key,float value,int num)
{
    if(!key)
        return;

    value*=0.7f;
    value+=0.3f;

    part_id id=get_part_id(key);
    if(id==invalid_part)
    {
        nya_log::log()<<"Unable to set character attribute: unknown key\n";
        return;
    }

    part &p=m_parts[id];
    if(num<0)
    {
        for(int i=0;i<2;++i)
            p.subparts[i].opacity=value;
    }

    if(num>=2)
        return;

    p.subparts[num].opacity=value;
}

float character::get_part_opacity(const char *key,int num)
{
    if(!key || num>=2)
        return -1.0f;

    if(num<0)
        num=0;

    part_id id=get_part_id(key);
    if(id==invalid_part)
    {
        nya_log::log()<<"Unable to set character attribute: unknown key\n";
        return -1.0f;
    }

    return m_parts[id].subparts[num].opacity;
}

void character::reset_parts_opacity()
{
    for(int i=0;i<max_parts;++i)
        for(int j=0;j<2;++j)
            m_parts[i].subparts[j].opacity=1.0f;
}

void character::draw(bool use_materials,bool cull_face)
{
    if(!m_parts[body].subparts[0].model.is_valid())
        return;

    //nya_render::set_color(m_color[0],m_color[1],m_color[2],1); //ToDO

    if(use_materials || m_parts[body].subparts[0].outline)
    {
        for(int i=0;i<m_body_group_count;++i)
        {
            if(i==m_body_blend_group_idx)
                continue;

            m_parts[body].subparts[0].model->draw(use_materials,cull_face,i);
        }
    }

    draw_part(eye,use_materials,cull_face);
    draw_part(hair,use_materials,cull_face);
    draw_part(head,use_materials,cull_face);

    //nya_render::set_color(m_color[0],m_color[1],m_color[2],1); //ToDO

    if(use_materials || m_parts[body].subparts[0].outline)
        m_parts[body].subparts[0].model->draw(use_materials,cull_face,m_body_blend_group_idx);

    for(int i=face;i<under;++i)
        draw_part(i,use_materials,cull_face);

    part &p=m_parts[under];
    for(int i=m_under_count;i>0;--i)
    {
        if(!use_materials && !p.subparts[i-1].outline)
            continue;

        int mode=m_under_state[i-1]?m_under_count:0;
        if(m_under_count==1 && m_under_state[1])
            mode+=2;

        model_ref &m=p.subparts[mode+i-1].model;
        if(m.is_valid())
        {
            //nya_render::set_color(m_color[0],m_color[1],m_color[2],p.subparts[i-1].opacity); //ToDO
            m->draw(use_materials,cull_face);
        }
    }

    for(int i=under+1;i<max_parts;++i)
        draw_part(i,use_materials,cull_face);
}

void character::draw_part(unsigned int idx,bool use_materials,bool cull_face)
{
    if(idx>=max_parts)
        return;

    part &p=m_parts[idx];
    for(int i=2;i>0;--i)
    {
        if(!use_materials && !p.subparts[i-1].outline)
            continue;

        model_ref &m=p.subparts[i-1].model;
        if(m.is_valid())
        {
            //nya_render::set_color(m_color[0],m_color[1],m_color[2],p.subparts[i-1].opacity); //ToDO
            m->draw(use_materials,cull_face);
        }
    }
}

void character::release()
{
    for(int i=0;i<max_parts;++i)
        m_parts[i].free_models();
}

character::part_id character::get_part_id(const char *name)
{
    if(!name)
        return invalid_part;

    switch(name[0])
    {
        case 'B': return body;
        case 'E': return eye;
        case 'U': return under;
        case 'A': return arm;
        case 'C': return costume;
        case 'N': return neck;
        case 'F': return face;

        case 'H':
        {
            if(name[1]=='A')
                return hair;

            return head;
        }

        case 'S':
        {
            if(name[1]=='O')
                return socks;

            return shoes;
        }
    };

    return invalid_part;
}

