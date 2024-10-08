//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "log/log.h"
#include "log/plain_file_log.h"
#include "system/app.h"
#include "system/system.h"
#include "render/render.h"

#include "resources/resources.h"
#include "resources/file_resources_provider.h"
#include "resources/composite_resources_provider.h"
#include "config.h"
#include "pl2_resources_provider.h"
#include "attributes.h"
#include "scene.h"
#include "ui.h"

#include "stdio.h"

#ifdef __APPLE__
    #include "TargetConditionals.h"
#endif

class npl2: public nya_system::app
{
private:
	bool on_splash()
	{
	    nya_log::log()<<"on_splash\n";
	    nya_render::set_clear_color(0,0.6,0.7,1);
	    nya_render::clear(true,false);
        return true;
    }

	void on_init()
	{
	    nya_log::log()<<"on_init\n";

        init_resource_system();

        /*
        nya_resources::resource_data *outline_cfg=nya_resources::get_resources_provider().access("outline_ignore_list.txt");
        if(outline_cfg)
        {
            get_outline_ignore().load(outline_cfg);
            outline_cfg->release();
        }
        */

const unsigned long time_start=nya_system::get_time();

        get_scene().init();

nya_log::log()<<"Scene init time: "<<nya_system::get_time()-time_start<<"\n";

	    m_ui.init();
	}

	void on_frame(unsigned int dt)
	{
        get_scene().process(dt);
        get_scene().draw();

	    m_ui.process(dt);
        m_ui.draw();

	    static unsigned int fps_counter=0;
	    static unsigned int fps_update_timer=0;

	    ++fps_counter;

	    fps_update_timer+=dt;
	    if(fps_update_timer>1000)
	    {
            char name[255];
            sprintf(name,"npl2 %d fps",fps_counter);
            set_title(name);

            //nya_log::log()<<name<<"\n";

            fps_update_timer%=1000;
            fps_counter=0;
	    }
	}

    void on_mouse_move(int x,int y)
    {
        //nya_log::log()<<"mmove "<<x<<" "<<y<<"\n";

        if(m_mouse_drag.left)
        {
            get_scene().get_camera().add_rot(((x-m_mouse_drag.last_x)*180.0f)/200.0f,
                                             -((y-m_mouse_drag.last_y)*180.0f)/200.0f);
        }
        else if(m_mouse_drag.right)
        {
            get_scene().get_camera().add_pos((x-m_mouse_drag.last_x)/20.0f,(y-m_mouse_drag.last_y)/20.0f);
        }
        else
            m_ui.mouse_move(x,y);

        m_mouse_drag.last_x=x;
        m_mouse_drag.last_y=y;
    }

    void on_mouse_scroll(int dx,int dy)
    {
        if(!m_ui.mouse_scroll(dx,dy))
            get_scene().get_camera().add_scale(dy*0.03f);
    }

    void on_mouse_button(nya_system::mouse_button button,bool pressed)
    {
        if(button==nya_system::mouse_left)
        {
            if(!m_ui.mouse_button(nya_ui::left_button,pressed) || !pressed)
                m_mouse_drag.left=pressed;
        }
        else if(button==nya_system::mouse_right)
            m_mouse_drag.right=pressed;
    }

    void on_resize(unsigned int w,unsigned int h)
    {
        //nya_log::log()<<"on_resize "<<w<<" "<<h<<"\n";

        if(!w || !h)
            return;

        nya_math::mat4 proj;
        proj.perspective(25,float(w)/h,5,1500);
        nya_render::set_projection_matrix(proj);

        m_ui.resize(w,h);
    }

	void on_free() 
    {
        nya_log::log()<<"on_free\n";
        
        get_scene().release();
        m_ui.release();
    }

private:
    void init_resource_system()
    {
        static bool init=false;
        if(init)
            return;

        init=true;

#if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        const std::string path=std::string(nya_system::get_app_path())+"../Documents/";
#else
        const std::string path=std::string(nya_system::get_app_path())+"add-ons/";
#endif

        static nya_resources::composite_resources_provider cprov;
        cprov.enable_cache();
        cprov.set_ignore_case(true);

        static nya_resources::file_resources_provider fprov;
        fprov.set_folder(path.c_str());

        const unsigned long time_start=nya_system::get_time();
        unsigned int arch_count=0;
        unsigned long arch_open_time=0;
        unsigned long arch_add_time=0;
        unsigned long attrib_time=0;

        static nya_memory::pool<nya_resources::pl2_resources_provider,32> pl2_providers;
        for(int i=0;i<fprov.get_resources_count();++i)
        {
            const char *name=fprov.get_resource_name(i);
            if(!nya_resources::check_extension(name,".pl2"))
                continue;

            nya_resources::pl2_resources_provider *prov=pl2_providers.allocate();

            unsigned long t0=nya_system::get_time();

            //nya_log::log()<<arch_name.c_str()<<"\n";
            prov->open_archieve(fprov.access(name));

            unsigned long t1=nya_system::get_time();

            cprov.add_provider(prov);

            unsigned long t2=nya_system::get_time();

            nya_resources::resource_data *attrib=prov->access_attribute();
            if(attrib)
            {
                get_attribute_manager().load(attrib);
                attrib->release();
            }

            unsigned long t3=nya_system::get_time();

            //nya_resources::log()<<arch_name.c_str()<<"\n";

            arch_open_time+=t1-t0;
            arch_add_time+=t2-t1;
            attrib_time+=t3-t2;
            ++arch_count;

            /*
            static unsigned long last_name_update_time=t3;
            static unsigned int name_update_timer=0;

            unsigned long dt=t3-last_name_update_time;
            last_name_update_time=t3;

            name_update_timer+=dt;
            if(name_update_timer>1000)
            {
                char name[255];
                sprintf(name,"npl2 %d archieves opened",arch_count);
                set_title(name);

                name_update_timer%=1000;
            }
            */
        }

        nya_log::log()<<"Archieves ("<<arch_count<<")"<<"and attribs load time: "<<nya_system::get_time()-time_start<<"\n";

        cprov.add_provider(&fprov);

        static nya_resources::file_resources_provider fprov2;
        fprov2.set_folder(nya_system::get_app_path(),false);

        cprov.add_provider(&fprov2);

        nya_resources::set_resources_provider(&cprov);

        nya_log::log()<<"Total res system init time: "<<nya_system::get_time()-time_start<<"\n";
        if(arch_count>0)
        {
            nya_log::log()<<"Average open archieve time: "<<float(arch_open_time)/arch_count<<"\n";
            nya_log::log()<<"Average add archieve time: "<<float(arch_add_time)/arch_count<<"\n";
            nya_log::log()<<"Average attrib time: "<<float(attrib_time)/arch_count<<"\n";
        }
    }

private:
    ui m_ui;

    struct mouse_drag
    {
        bool left;
        bool right;
        int last_x;
        int last_y;

        mouse_drag():left(false),right(false)
                    ,last_x(0),last_y(0) {}
    } m_mouse_drag;
};

#include "memory/tmp_buffer.h"

void debug_write_data(nya_resources::resource_data *data,const char *name)
{
    if(!data)
        return;

    FILE *out=fopen(name,"wb+");
    if(!out)
        return;

    nya_memory::tmp_buffer_scoped buf(data->get_size());
    data->read_all(buf.get_data());
    fwrite(buf.get_data(),data->get_size(),1,out);
    data->release();
    fclose(out);
}

void debug_extract_pl2(const char *name)
{
    if(!name)
        return;

    const std::string path=std::string(nya_system::get_app_path())+"add-ons/";

    nya_resources::file_resources_provider fprov;
    fprov.set_folder(path.c_str());
    nya_resources::resource_data *arch_data=fprov.access(name);
    if(!arch_data)
        return;

    nya_resources::pl2_resources_provider arch;
    arch.open_archieve(arch_data);
    for(int i=0;i<arch.get_resources_count();++i)
    {
        const char *res_name=arch.get_resource_name(i);
        if(!res_name)
            continue;

        const std::string n=std::string("pl2_out/")+name+"_"+res_name;
        debug_write_data(arch.access(res_name),n.c_str());
    }

    const std::string n=std::string("pl2_out/")+name+"_"+"attribute.txt";
    debug_write_data(arch.access_attribute(),n.c_str());

    arch.close_archieve();
}

int main(int argc, char **argv)
{
    //debug_extract_pl2("sc01.pl2");
    //debug_extract_pl2("sc01_res.pl2");
    //debug_extract_pl2("sc02_res.pl2");
    //debug_extract_pl2("sounds.pl2");
    //debug_extract_pl2("sc01sm.pl2");
    //return 0;

    const std::string log_file_name=std::string(nya_system::get_app_path())+"log.txt";
    nya_log::plain_file_log log;
    log.open(log_file_name.c_str());

    //nya_log::set_log(&log);

    nya_log::log()<<"npl2 started from path "<<nya_system::get_app_path()<<"\n";

    const std::string config_path=std::string(nya_system::get_app_path())+"npl2.cfg";
    nya_resources::resource_data *cfg=
    nya_resources::get_resources_provider().access(config_path.c_str());
    if(cfg)
    {
        get_config().load(cfg);
        cfg->release();
        nya_log::log()<<"config loaded, starting\n";
    }
    else
        nya_log::log()<<"unable to load config\n";

    npl2 app;
    app.set_title("Loading, please wait...");
    app.start_windowed(100,100,640,480,get_config().antialiasing);

    log.close();

    return 0;
}
