//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "scene.h"
#include "config.h"
#include "attributes.h"
#include "resources/resources.h"
#include "tsb_anim.h"
#include "render/render.h"

#include "string.h"
#include "stdlib.h"

#include <string>

#include "math/matrix.h"

const nya_math::mat4 &viewer_camera::get_matrix()
{
    if(!m_recalc_mat)
        return m_mat;

    m_mat.identity();
    m_mat.translate(0,0,-45);
    m_mat.scale(m_scale);
    m_mat.translate(m_pos_x,m_pos_y,0);
    m_mat.rotate(m_rot_y,1,0,0);
    m_mat.rotate(m_rot_x,0,1,0);
    m_mat.translate(0,-8,0);

    m_recalc_mat=false;

    return m_mat;
}

void viewer_camera::add_rot(float dx,float dy)
{
    m_rot_x+=dx;
    m_rot_y+=dy;

    const float max_angle=360.0f;

    if ( m_rot_x > max_angle )
        m_rot_x -= max_angle;

    if ( m_rot_x < -max_angle )
        m_rot_x += max_angle;

    if ( m_rot_y > max_angle )
        m_rot_y -= max_angle;

    if ( m_rot_y < -max_angle )
        m_rot_y += max_angle;
    
    m_recalc_mat=true;

}

void viewer_camera::add_pos(float dx,float dy)
{
    m_pos_x+=dx;
    m_pos_y+=dy;
    
    m_recalc_mat=true;
}

void viewer_camera::add_scale(float ds)
{
    m_scale *= (1.0f+ds);
    const float min_scale=0.4f;
    if(m_scale<min_scale)
        m_scale=min_scale;

    const float max_scale=10.0f;
    if(m_scale>max_scale)
        m_scale=max_scale;
    
    m_recalc_mat=true;
}

void scene::init()
{
#ifndef OPENGL_ES
    get_shared_anims().should_unload_unused(false);
#endif

    //get_shared_models().set_lru_limit(20);
    //get_shared_models().should_unload_unused(false);

    m_anim_list.push_back("event_01");
    m_anim_list.push_back("event_02");
    m_anim_list.push_back("event_03");
    m_anim_list.push_back("event_04");
    m_anim_list.push_back("event_05");
    m_anim_list.push_back("event_06");
    //m_anim_list.push_back("umauma00_F");

    std::vector<char> script_buf;
    std::string script_str;

    bool anim_ignored=false;
    std::map<std::string,bool> anim_map;
    for(size_t i=0;i<m_anim_list.size();++i)
        anim_map[m_anim_list[i].name[0]]=true;

    for(int i=0;i<nya_resources::get_resources_provider().get_resources_count();++i)
    {
        const char *name=nya_resources::get_resources_provider().get_resource_name(i);
        if(nya_resources::check_extension(name,".txt"))
        {
            nya_resources::resource_data *data=nya_resources::get_resources_provider().access(name);
            if(data)
            {
                script_buf.resize(data->get_size());
                data->read_all(&script_buf[0]);
                script_buf.push_back(0);
                script_str.assign(&script_buf[0]);

                size_t pos=script_str.find("%mm");
                while(pos!=std::string::npos)
                {
                    if(pos+5>=script_str.size())
                        break;

                    size_t pos2=script_str.find(";",pos+5);
                    if(pos2==std::string::npos)
                        break;

                    const std::string anim_name=script_str.substr(pos+5,pos2-(pos+5));
                    //nya_log::log()<<"anim: "<<anim_name.c_str()<<"\n";

                    char num = script_str[pos+3];
                    if(num=='0')
                    {
                        if(anim_map.find(anim_name)==anim_map.end())
                        {
                            m_anim_list.push_back(anim_name.c_str());
                            anim_map[anim_name]=true;

                            //nya_log::log()<<"anim0: "<<anim_name.c_str()<<"\n";

                            size_t loc_pos=script_str.rfind("%mp0,",pos);
                            if(loc_pos!=std::string::npos)
                            {
                                size_t loc_pos2=script_str.find(";",loc_pos);
                                if(loc_pos2>(loc_pos+7))
                                {
                                    std::string pos=script_str.substr(loc_pos+5,loc_pos2-(loc_pos+5));
                                    m_anim_list.back().loc_idx[0]=atoi(&pos[pos.length()-2]);
                                    //nya_log::log()<<"\tpos0: "<<pos.c_str()<<" "<<m_anim_list.back().loc_idx<<"\n";
                                }
                            }

                            anim_ignored=false;
                        }
                        else
                            anim_ignored=true;
                    }
                    else if(num<='9' && !anim_ignored)
                    {
                        size_t model_pos=script_str.rfind("%ml",pos);
                        if(model_pos!=std::string::npos)
                        {
                            char num = script_str[model_pos+3];
                            size_t model_pos2=script_str.find(";",model_pos);
                            if(model_pos2>(model_pos+7))
                            {
                                std::string model=script_str.substr(model_pos+5,model_pos2-(model_pos+5));
                                m_anim_list.back().model_name[num-'1'+1]=model;
                            }
                        }

                        m_anim_list.back().name[num-'1'+1]=anim_name;

                        size_t loc_pos=script_str.rfind("%mp",pos);
                        if(loc_pos!=std::string::npos)
                        {
                            char num = script_str[loc_pos+3];
                            size_t loc_pos2=script_str.find(";",loc_pos);
                            std::string pos=script_str.substr(loc_pos+5,loc_pos2-(loc_pos+5));
                            m_anim_list.back().loc_idx[num-'1'+1]=atoi(&pos[pos.length()-2]);
                            //nya_log::log()<<"\tpos: "<<pos.c_str()<<" "<<m_anim_list.back().loc_idx[num-'1'+1]<<"\n";
                        }
                    }

                    pos=script_str.find("%mm",pos2);
                }

                data->release();
            }
        }
    }

    m_curr_anim = m_anim_list.begin();
    if(m_curr_anim!=m_anim_list.end())
        m_imouto.set_anim(m_curr_anim->name[0].c_str());

    nya_resources::resource_data *model_res = nya_resources::get_resources_provider().access("ani_bodyA_00.tmb");
    if(model_res)
    {
        m_aniki.load(model_res);
        model_res->release();
    }

    const char *uber_vs=
                            "uniform mat4 bones[200];"

                            "varying vec4 tc;"
                            "varying vec4 normal;"
                            "varying vec4 vcolor;"

                            "void main()"
                            "{"
                            "  tc=gl_MultiTexCoord0;"
                            "  vec4 bone_idx=gl_MultiTexCoord1;"
                            "  vec4 bone_weight=gl_MultiTexCoord2;"

                            "  const float eps=0.01;"

                            "  mat4 bone=bones[int(bone_idx.x)]*bone_weight.x;"

                            "  if(bone_weight.y>eps)"
                            "    bone+=bones[int(bone_idx.y)]*bone_weight.y;"

                            "  if(bone_weight.z>eps)"
                            "    bone+=bones[int(bone_idx.z)]*bone_weight.z;"

                            "  if(bone_weight.w>eps)"
                            "    bone+=bones[int(bone_idx.w)]*bone_weight.w;"

                            "\n#if defined(specular_enabled) || defined(outlines)\n"
                            "  vec4 n=bone*vec4(gl_Normal.xyz,0);"
                            "  normal=gl_ModelViewMatrix*n;"
                            "\n#endif\n"

                            "\n#ifdef vcolor_enabled\n"
                            "  vcolor=gl_MultiTexCoord3;"
                            "\n#endif\n"
                            "\n#ifdef outlines\n"
                            "  gl_Position=gl_ModelViewProjectionMatrix*(bone*(gl_Vertex+vec4(gl_Normal.xyz*0.02,0.0)));"
                            "\n#else\n"
                            "  gl_Position=gl_ModelViewProjectionMatrix*bone*gl_Vertex;"
                            "\n#endif\n"
                            "}";

    const char *vprogram=
                             "mat3 get_rot(mat4 m)"
                             "{"
                             "  return mat3(m[0].xyz,m[1].xyz,m[2].xyz);"
                             "}"

                             "varying vec4 tc;"
                             "varying vec4 normal;"
                             "varying vec4 vcolor;"

                             "void main()"
                             "{"
                             "  tc=gl_MultiTexCoord0;"
                             "  vcolor=gl_MultiTexCoord3;"

                             //"  normal.xyz=get_rot(gl_ModelViewMatrix)*gl_Normal.xyz;"

                             "  gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;"
                             "}";
    
    const char *fprogram=
                            "uniform sampler2D base_map;"

                            "varying vec4 tc;"
                            "varying vec4 normal;"
                            "varying vec4 vcolor;"

                            "void main(void)"
                            "{"
                            "  vec4 base=texture2D(base_map,tc.xy);"

                            //"  float l=dot(normalize(vec3(0,0,1.0)),normalize(gl_TexCoord[1].xyz));"
                            //"  float ls=dot(normalize(vec3(-0.3,0,1.0)),normalize(gl_TexCoord[1].xyz));"
                            //"  gl_FragColor=vec4((0.15+max(0.0,l*0.85))*base.rgb+pow(max(ls,0.0),90.0)*vec3(0.06),base.a);"
                            //"  gl_FragColor=vec4(base.rgb+pow(max(ls,0.0),90.0)*vec3(0.06),base.a);"

                            "  vec4 frag=base*vcolor;"
                            "  if(frag.a<0.5) discard;"
                            "  gl_FragColor=frag;"
                            //"  gl_FragColor=vcolor;"
                            "}";

    m_shader_scenery.add_program(nya_render::shader::vertex,vprogram);
    m_shader_scenery.add_program(nya_render::shader::pixel,fprogram);

    std::string scenery_anim_vs_str;
    std::string scenery_anim_ps_str;

    scenery_anim_vs_str.append("#define vcolor_enabled\n");
    scenery_anim_vs_str.append(uber_vs);

    m_shader_scenery_anim.add_program(nya_render::shader::vertex,scenery_anim_vs_str.c_str());
    m_shader_scenery_anim.add_program(nya_render::shader::pixel,fprogram);

    m_shsc_mat_uniform=m_shader_scenery_anim.find_uniform("bones");

    const char *char_ps=
                        "uniform sampler2D base_map;"

                        "varying vec4 tc;"
                        "varying vec4 normal;"
                        "uniform vec4 color;"
                        "varying vec4 vcolor;"

                         "void main(void)"
                         "{"
                         "  vec4 base=texture2D(base_map,tc.xy);"

                         "\n#ifdef specular_enabled\n"
                         //"  float l=dot(normalize(vec3(0,0,1.0)),normalize(gl_TexCoord[1].xyz));"
                         "  float ls=dot(normalize(vec3(-0.3,0,1.0)),normalize(normal.xyz));"
                         //"  gl_FragColor=vec4((0.85+max(0.0,l*0.15))*base.rgb+pow(max(ls,0.0),90.0)*vec3(0.06),base.a);"
                         "  vec4 frag=vec4(base.rgb+pow(max(ls,0.0),90.0)*vec3(0.06),base.a)*color;"
                         "\n#else\n"
                         "  vec4 frag=base*color;"
                         "\n#endif\n"
                        "  if(frag.a<0.2) discard;"
                        "  gl_FragColor=frag;"
                         //"  gl_FragColor=base;"
                         "}";

    std::string char_vs_str;
    std::string char_ps_str;

    if(get_config().specular_enabled)
    {
        const char *specular_define="#define specular_enabled\n";
        char_vs_str.append(specular_define);
        char_ps_str.append(specular_define);
    }

    char_vs_str.append(uber_vs);
    char_ps_str.append(char_ps);

    m_shader.add_program(nya_render::shader::vertex,char_vs_str.c_str());
    m_shader.add_program(nya_render::shader::pixel,char_ps_str.c_str());

    m_sh_mat_uniform=m_shader.find_uniform("bones");
    m_sh_color=m_shader.find_uniform("color");

    m_shader_black.add_program(nya_render::shader::vertex,(std::string("#define outlines\n")+uber_vs).c_str());
    m_shader_black.add_program(nya_render::shader::pixel,
                               "uniform sampler2D base_map;"
                               "varying vec4 tc;"

                               "void main(void)"
                               "{"
                               "  vec4 c=texture2D(base_map,tc.xy);"
                               "  if(c.a<0.2) discard;"
                               "  gl_FragColor=vec4(0.0,0.0,0.0,c.a*0.3);"
                               "}");
    m_shbl_mat_uniform=m_shader_black.find_uniform("bones");

    m_imouto.set_attrib("COORDINATE","fudan05");
    //m_imouto.set_attrib("COORDINATE","zok_panya00");
    //set_bkg("room_office_000");
    m_camera.add_rot(0,10);
}

void scene::set_bkg(const char *name)
{
    for(int i=0;i<max_bkg_models;++i)
        m_bkg_models[i].release();

    m_has_scenery=false;

    attribute *atr=get_attribute_manager().get("BG",name);
    if(!atr)
        return;

    //atr->debug_print();

    for(int i=0;i<max_bkg_models;++i)
    {
        char key[16]="FILE_0";
        key[5]='0'+i*2;

        m_bkg_anims[i].clear();

        const char *name=atr->get_value(key);
        if(!name || strcmp(name,"nil")==0)
            continue;

        nya_resources::resource_data *scenery_res = nya_resources::get_resources_provider().access(name);
        if(scenery_res)
        {
            m_bkg_models[i].load(scenery_res);
            scenery_res->release();
        }

        ++key[5];

        name=atr->get_value(key);
        if(!name || strcmp(name,"nil")==0)
            continue;

        nya_log::log()<<"scenery anim: "<<name<<"\n";

        scenery_res = nya_resources::get_resources_provider().access(name);
        if(scenery_res)
        {
            tsb_anim anim;
            anim.load(scenery_res);
            m_bkg_anims[i].apply_anim(m_bkg_models[i],anim);
            anim.release();
            scenery_res->release();
        }

        m_bkg_models_anim_times[i]=0;
    }

    m_has_scenery=true;
}

void scene::process(unsigned int dt)
{
    if(dt>3000)
       dt=3000;

    const float kdt=dt*0.001f;

    const float anim_framerate=50.0f;

    m_anim_time+=kdt*anim_framerate;

    character &imouto=m_preview?m_imouto_preview:m_imouto;

    const size_t frames_count=imouto.get_frames_count();
    if(m_anim_time>=frames_count)
    {
        m_anim_time-=frames_count-imouto.get_first_loop_frame();
        if(m_anim_time>=frames_count)
            m_anim_time=imouto.get_first_loop_frame();
    }

    for(int i=0;i<max_bkg_models;++i)
    {
        const unsigned int frames_count=m_bkg_anims[i].get_frames_count();
        if(!frames_count)
            continue;

        m_bkg_models_anim_times[i]+=kdt*anim_framerate;
        while(m_bkg_models_anim_times[i]>=frames_count)
            m_bkg_models_anim_times[i]-=frames_count;
    }
}

void scene::draw()
{
    if(m_has_scenery)
        nya_render::set_clear_color(0,0,0,0);
    else
        nya_render::set_clear_color(0.2,0.4,0.5,0);

    nya_render::clear(true,true);

    character &imouto=m_preview?m_imouto_preview:m_imouto;

    nya_render::depth_test::enable(nya_render::depth_test::less);
    nya_render::blend::enable(nya_render::blend::src_alpha,nya_render::blend::inv_src_alpha);
    apply_color(1.0,1.0,1.0,1.0);

    const tmb_model::locator *scene_loc=0;
    if(!m_anim_list.empty() && m_curr_anim!=m_anim_list.end())
        scene_loc=m_bkg_models[0].get_locator(m_curr_anim->loc_idx[0]);

    if(scene_loc)
    {
        nya_math::mat4 mat=m_camera.get_matrix();

        mat.rotate(-scene_loc->ang[1]*180.0f/3.14f,0,1,0);
        mat.translate(-scene_loc->pos[0],-scene_loc->pos[1],-scene_loc->pos[2]);

        nya_render::set_modelview_matrix(mat);
    }

    for(int i=0;i<max_bkg_models;++i)
    {
        const unsigned int bones_count=m_bkg_models[i].get_bones_count();
        const unsigned int frames_count=m_bkg_anims[i].get_frames_count();

        if(bones_count && frames_count)
        {
            m_shader_scenery_anim.bind();
            m_shader_scenery_anim.set_uniform16_array(m_shsc_mat_uniform,
                                                      m_bkg_anims[i].get_buffer(int(m_bkg_models_anim_times[i])),
                                                      bones_count);
            m_bkg_models[i].draw(true,true);
            m_shader_scenery_anim.unbind();
        }
        else
        {
            m_shader_scenery.bind();
            m_bkg_models[i].draw(true,true);
            m_shader_scenery.unbind();
        }
    }

    nya_render::set_modelview_matrix(m_camera.get_matrix());

    const size_t frames_count=imouto.get_frames_count();
    if(frames_count)
    {
        m_shader.bind();
        m_shader.set_uniform16_array(m_sh_mat_uniform,
                                     imouto.get_buffer(int(m_anim_time)),
                                     imouto.get_bones_count());
    }

    if(scene_loc)
        imouto.set_color(scene_loc->color[0],scene_loc->color[1],scene_loc->color[2]);

    //ToDo: remove
    if(scene_loc)
        apply_color(scene_loc->color[0],scene_loc->color[1],scene_loc->color[2],1.0f);
    else
        apply_color(1.0f,1.0f,1.0f,1.0f);

    imouto.draw(true,true);

    if(scene_loc)
        apply_color(scene_loc->color[0],scene_loc->color[1],scene_loc->color[2],1.0f);
    else
        apply_color(1.0f,1.0f,1.0f,1.0f);

    /*
    glPushMatrix();
    if(m_curr_anim!=m_anim_list.end() && m_curr_anim->loc_idx[1]!=m_curr_anim->loc_idx[0])
    {
        if(scene_loc)
        {
            glRotatef(-scene_loc->ang[1]*180.0f/3.14f,0.0f,1.0f,0.0f);
            glTranslatef(-scene_loc->pos[0],-scene_loc->pos[1],-scene_loc->pos[2]);

            const tmb_model::locator *bro_loc=m_bkg_models[0].get_locator(m_curr_anim->loc_idx[1]);
            if(bro_loc)
            {
                glRotatef(-bro_loc->ang[1]*180.0f/3.14f,0.0f,1.0f,0.0f);
                glTranslatef(-bro_loc->pos[0],-bro_loc->pos[1],-bro_loc->pos[2]);
            }
        }
    }*/

    //bro
    const size_t bro_frames_count=m_aniki_anim.get_frames_count();
    if(bro_frames_count)
    {
        m_shader.set_uniform16_array(m_sh_mat_uniform,
                                     m_aniki_anim.get_buffer(int(m_anim_time)),
                                     m_aniki_anim.get_bones_count());
        m_aniki.draw(true,true);
    }/*
    glPopMatrix();

    glPushMatrix();
    if(m_curr_anim!=m_anim_list.end() && m_curr_anim->loc_idx[2]!=m_curr_anim->loc_idx[0])
    {
        if(scene_loc)
        {
            //glTranslatef(scene_loc->pos[0],scene_loc->pos[1],scene_loc->pos[2]);
            //glRotatef(scene_loc->ang[1]*180.0f/3.14f,0.0f,1.0f,0.0f);
            glRotatef(-scene_loc->ang[1]*180.0f/3.14f,0.0f,1.0f,0.0f);
            glTranslatef(-scene_loc->pos[0],-scene_loc->pos[1],-scene_loc->pos[2]);

            const tmb_model::locator *third_loc=m_bkg_models[0].get_locator(m_curr_anim->loc_idx[2]);
            if(third_loc)
            {
                glTranslatef(third_loc->pos[0],third_loc->pos[1],third_loc->pos[2]);
                glRotatef(third_loc->ang[1]*180.0f/3.14f,0.0f,1.0f,0.0f);
            }
        }
    }*/

    //another bro or item
    const size_t third_frames_count=m_the_third_anim.get_frames_count();
    if(third_frames_count)
    {
        m_shader.set_uniform16_array(m_sh_mat_uniform,
                                     m_the_third_anim.get_buffer(int(m_anim_time)),
                                     m_the_third_anim.get_bones_count());
        m_the_third.draw(true,true);
    }
    //glPopMatrix();

    if(frames_count)
        m_shader.unbind();

    if(get_config().wireframe_outline_enabled)
    {
        //nya_render::blend::enable(nya_render::blend::src_alpha,nya_render::blend::inv_src_alpha);
        //m_scenery.draw(false);

        if(frames_count)
        {
            m_shader_black.bind();
            m_shader_black.set_uniform16_array(m_shbl_mat_uniform,
                                               imouto.get_buffer(int(m_anim_time)),
                                               imouto.get_bones_count());
        }

        imouto.draw(true,false);

        /*
        glPushMatrix();
        if(m_curr_anim!=m_anim_list.end() && m_curr_anim->loc_idx[1]!=m_curr_anim->loc_idx[0])
        {
            if(scene_loc)
            {
                glRotatef(-scene_loc->ang[1]*180.0f/3.14f,0.0f,1.0f,0.0f);
                glTranslatef(-scene_loc->pos[0],-scene_loc->pos[1],-scene_loc->pos[2]);
                
                const tmb_model::locator *bro_loc=m_bkg_models[0].get_locator(m_curr_anim->loc_idx[1]);
                if(bro_loc)
                {
                    glTranslatef(bro_loc->pos[0],bro_loc->pos[1],bro_loc->pos[2]);
                    glRotatef(bro_loc->ang[1]*180.0f/3.14f,0.0f,1.0f,0.0f);
                }
            }
        }*/
        
        //bro
        const size_t bro_frames_count=m_aniki_anim.get_frames_count();
        if(bro_frames_count)
        {
            m_shader_black.set_uniform16_array(m_shbl_mat_uniform,
                                               m_aniki_anim.get_buffer(int(m_anim_time)),
                                               m_aniki_anim.get_bones_count());
            m_aniki.draw(true,false);
        }
        //glPopMatrix();

        /*
        //another bro or item
        const size_t third_frames_count=m_the_third.get_frames_count();
        if(third_frames_count)
        {
            m_shader.set_uniform16_array(m_sh_mat_uniform,
                                         m_the_third.get_buffer(int(m_anim_time)),
                                         m_the_third.get_bones_count());
            m_the_third.draw(true,false);
        }
        */

        if(frames_count)
            m_shader_black.unbind();
    }
}

void scene::set_imouto_attr(const char *key,const char *value,int num)
{
    m_imouto.set_attrib(key,value,num);
}

void scene::set_imouto_preview(const char *key,const char *value,int num)
{
    m_imouto_preview.copy_attrib(m_imouto);
    m_imouto_preview.set_attrib(key,value,num);
    m_imouto_preview.set_anim(m_imouto.get_anim());
    m_preview=true;
}

void scene::set_imo_under_state(bool top,bool bottom)
{
    m_imouto.set_under_state(top,bottom);
}

void scene::set_part_opacity(const char *key,float value,int num)
{
    m_imouto.set_part_opacity(key,value,num);
}

const char *scene::get_imouto_attr(const char *key,int num)
{
    return m_imouto.get_attrib(key,num);
}

float scene::get_part_opacity(const char *key,int num)
{
    return m_imouto.get_part_opacity(key,num);
}

void scene::reset_parts_opacity()
{
    m_imouto.reset_parts_opacity();
}

void scene::finish_imouto_preview()
{
    m_preview=false;
}

void scene::prev_anim()
{
    if(m_anim_list.empty())
        return;

    if(m_curr_anim-- == m_anim_list.begin())
        m_curr_anim= --m_anim_list.end();

    apply_anim();
}

void scene::next_anim()
{
    if(m_anim_list.empty())
        return;

    if(++m_curr_anim==m_anim_list.end())
        m_curr_anim=m_anim_list.begin();

    apply_anim();
}

void scene::set_anim(unsigned int num)
{
    if(num>=m_anim_list.size())
        return;

    m_curr_anim=m_anim_list.begin()+num;

    nya_log::log()<<"\nset_anim\n";

    for(int i=0;i<10;++i)
    {
        if(!m_curr_anim->name[i].empty())
            nya_log::log()<<m_curr_anim->name[i].c_str()<<"\n";
    }

    apply_anim();
}

const char *scene::get_anim_name(unsigned int num)
{
    if(num>=m_anim_list.size())
        return 0;

    return m_anim_list[num].name[0].c_str();
}

unsigned int scene::get_anims_count()
{
    return (unsigned int)m_anim_list.size();
}

void scene::apply_anim()
{
    if(m_anim_list.empty() && m_curr_anim!=m_anim_list.end())
        return;

    m_imouto.set_anim(m_curr_anim->name[0].c_str());
    m_anim_time=0;

    std::string bro_anim=m_curr_anim->name[1];
    if(!bro_anim.empty())
    {
        bro_anim.append(".tsb");

        anim_ref a=get_shared_anims().access(bro_anim.c_str());
        if(a.is_valid())
            m_aniki_anim.apply_anim(m_aniki,*a.const_get());
        else
            m_aniki_anim.clear();
    }
    else
        m_aniki_anim.clear();

    std::string third_anim=m_curr_anim->name[2];
    std::string third_model=m_curr_anim->model_name[2];
    if(!third_anim.empty() && !third_model.empty())
    {
        third_anim.append(".tsb");
        third_model.append(".tmb");
        
        anim_ref a=get_shared_anims().access(third_anim.c_str());
        m_the_third.release();
        nya_resources::resource_data *model_res=nya_resources::get_resources_provider().access(third_model.c_str());
        if(model_res)
        {
            m_the_third.load(model_res);
            model_res->release();
            m_the_third_anim.apply_anim(m_the_third,*a.const_get());
        }
        else
        {
            m_the_third.release();
            m_the_third_anim.clear();
        }
    }
    else
        m_the_third.release();
}

void scene::apply_color(float r,float g,float b,float a) { m_shader.set_uniform(m_sh_color,r,g,b,a); }

void scene::release()
{
    for(int i=0;i<max_bkg_models;++i)
        m_bkg_models[i].release();

    m_imouto.release();
    m_imouto_preview.release();
    m_aniki.release();

    m_shader.release();
    m_shader_scenery.release();

    //get_shared_anims().free_all();
}

scene &get_scene()
{
    static scene scene;
    return scene;
}
