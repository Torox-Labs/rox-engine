//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "system/app.h"
#include "system/system.h"
#include "render/vbo.h"
#include "render/shader.h"
#include "render/render.h"
#include "system/shaders_cache_provider.h"
#include "log/log.h"
#include "log/android_log.h"
#include <sstream>

class test_cube: public nya_system::app
{
private:
	bool on_splash()
	{
	    nya_log::log()<<"on_splash\n";

		nya_render::set_clear_color(0.0f,0.6f,0.7f,1.0f);
		nya_render::clear(true,true);

        return true;
    }

	void on_init()
	{
	    nya_log::log()<<"on_init\n";

	    nya_render::set_clear_color(0.2f,0.4f,0.5f,0.0f);
		nya_render::set_clear_depth(1.0f);
        nya_render::depth_test::enable(nya_render::depth_test::less);

		float vertices[] = 
		{
			-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
		    -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 1.0f,
			-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
			-0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 1.0f,
			 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
			 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 1.0f,
			 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
			 0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f
		};

		unsigned short indices[] = 
		{
			0,2,1, 1,2,3, // -x
			4,5,6, 5,7,6, // +x
			0,1,5, 0,5,4, // -y
			2,6,7, 2,7,3, // +y
			0,4,6, 0,6,2, // -z
			1,3,7, 1,7,5, // +z
		};

		m_vbo.set_vertex_data(vertices,sizeof(float)*6,8);
		m_vbo.set_vertices(0,3);
		m_vbo.set_colors(sizeof(float)*3,3);
        m_vbo.set_index_data(indices,nya_render::vbo::index2b,
			     sizeof(indices)/sizeof(unsigned short));

        /*
        if(nya_render::get_render_api()==nya_render::render_api_directx11)
        {
            static nya_system::compiled_shaders_provider csp;
            csp.set_load_path( nya_system::get_app_path() );
            csp.set_save_path( nya_system::get_app_path() );
            nya_render::set_compiled_shaders_provider( &csp );
        }
        */

		const char *vs_code=
			"varying vec4 color;"
			"void main()"
			"{"
				"color=gl_Color;"
                "gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;"
			"}";

		const char *ps_code=
			"varying vec4 color;"
			"void main()"
			"{"
				"gl_FragColor=color;"
			"}";

		m_shader.add_program(nya_render::shader::vertex,vs_code);
		m_shader.add_program(nya_render::shader::pixel,ps_code);
	}

	void on_frame(unsigned int dt)
	{
        nya_render::clear(true,true);

		m_rot+=dt*0.05f;
        if(m_rot>360)
            m_rot=0;

		nya_math::mat4 mv;
		mv.translate(0,0,-2.0f);
		mv.rotate(30.0f,1.0f,0.0f,0.0f);
		mv.rotate(m_rot,0.0f,1.0f,0.0f);
		nya_render::set_modelview_matrix(mv);

		m_shader.bind();
		m_vbo.bind();
		m_vbo.draw();
		m_vbo.unbind();
		m_shader.unbind();

	    static unsigned int fps_counter=0,fps_update_timer=0;
	    ++fps_counter;
	    fps_update_timer+=dt;
	    if(fps_update_timer>1000)
	    {
            std::ostringstream os;
            os<<"test cube "<<fps_counter<<" fps";
            std::string str=os.str();
            set_title(str.c_str());

            fps_update_timer%=1000;
            fps_counter=0;
	    }
	}

    void on_resize(unsigned int w,unsigned int h)
    {
        nya_log::log()<<"on_resize "<<w<<" "<<h<<"\n";

        if(!w || !h)
            return;

		nya_math::mat4 proj;
		proj.perspective(70.0f,float(w)/h,0.01f,100.0f);
		nya_render::set_projection_matrix(proj);
    }

	void on_free()
	{
		nya_log::log()<<"on_free\n";

		m_vbo.release();
		m_shader.release();
	}

	void on_keyboard(unsigned int key,bool pressed)
	{
		if((key==nya_system::key_back || key==nya_system::key_escape) && !pressed)
			finish();
	}

public:
	test_cube(): m_rot(0.0f) {}

private:
    nya_render::vbo m_vbo;
	nya_render::shader m_shader;
	float m_rot;
};

#ifdef _WIN32
  #ifdef WINDOWS_METRO
    int main( Platform::Array<Platform::String^>^ args)
  #else
    int CALLBACK WinMain(HINSTANCE,HINSTANCE,LPSTR,int)
  #endif
#else
	int main(int argc, char **argv)
#endif
{

#ifdef __ANDROID__
        nya_log::set_log(new nya_log::android_log("test_cube"));
#endif

    test_cube app;
    app.set_title("Loading, please wait...");
    app.start_windowed(100,100,640,480,0);
    nya_log::log()<<"exit success\n";

    return 0;
}
