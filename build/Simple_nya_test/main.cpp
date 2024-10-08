#include "system/app.h"
#include "system/system.h"
#include "render/vbo.h"
#include "render/shader.h"
#include "render/render.h"
#include "system/shaders_cache_provider.h"
#include "log/log.h"
#include "log/android_log.h"
#include <sstream>

#include "render/render_opengl_ext.h"

// INCLUDE SDL
#include <SDL3/SDL.h>
#include <SDL3/SDL_mouse.h>

// Include Assimp Headers
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class test_cube : public nya_system::app
{
private:

	SDL_Window* window;
	SDL_Renderer* renderer;

	bool on_splash()
	{
		nya_log::log() << "on_splash\n";

		nya_render::set_clear_color(0.0f, 0.6f, 0.7f, 1.0f);
		nya_render::clear(true, true);

		return true;
	}

	void load_object_model(const char* objFilePath)
	{
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(objFilePath, aiProcess_Triangulate | aiProcess_GenSmoothNormals);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			nya_log::log() << "Error loading .obj file: " << importer.GetErrorString() << "\n";
			return;
		}

		const aiMesh* mesh = scene->mMeshes[0]; // Assuming there's only one mesh in the scene

		float* vertices = new float[mesh->mNumVertices * 6]; // Assuming each vertex has 3 positions and 3 normals
		for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
		{
			vertices[i * 6] = mesh->mVertices[i].x;
			vertices[i * 6 + 1] = mesh->mVertices[i].y;
			vertices[i * 6 + 2] = mesh->mVertices[i].z;
			vertices[i * 6 + 3] = mesh->mNormals[i].x;
			vertices[i * 6 + 4] = mesh->mNormals[i].y;
			vertices[i * 6 + 5] = mesh->mNormals[i].z;
		}

		unsigned short* indices = new unsigned short[mesh->mNumFaces * 3]; // Assuming each face is a triangle
		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			indices[i * 3] = mesh->mFaces[i].mIndices[0];
			indices[i * 3 + 1] = mesh->mFaces[i].mIndices[1];
			indices[i * 3 + 2] = mesh->mFaces[i].mIndices[2];
		}

		m_vbo.set_vertex_data(vertices, sizeof(float) * 6, mesh->mNumVertices);
		m_vbo.set_vertices(0, 3);
		m_vbo.set_colors(sizeof(float) * 3, 3);
		m_vbo.set_index_data(indices, nya_render::vbo::index2b, mesh->mNumFaces * 3);

		delete[] vertices;
		delete[] indices;
	}

	void on_init()
	{
		nya_log::log() << "on_init\n";

		nya_render::set_clear_color(0.2f, 0.4f, 0.5f, 0.0f);
		nya_render::set_clear_depth(1.0f);
		nya_render::depth_test::enable(nya_render::depth_test::less);

		// Load .obj model
		const char* objFilePath = "C:\\Users\\Aninoss\\Downloads\\obj\\Gear2.obj";
		load_object_model(objFilePath);

		/*float vertices[] =
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

		m_vbo.set_vertex_data(vertices, sizeof(float) * 6, 8);
		m_vbo.set_vertices(0, 3);
		m_vbo.set_colors(sizeof(float) * 3, 3);
		m_vbo.set_index_data(indices, nya_render::vbo::index2b,
			sizeof(indices) / sizeof(unsigned short));*/

		/*
		if(nya_render::get_render_api()==nya_render::render_api_directx11)
		{
			static nya_system::compiled_shaders_provider csp;
			csp.set_load_path( nya_system::get_app_path() );
			csp.set_save_path( nya_system::get_app_path() );
			nya_render::set_compiled_shaders_provider( &csp );
		}
		*/

		const char* vs_code =
			"varying vec4 color;"
			"void main()"
			"{"
			"color=gl_Color;"
			"gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;"
			"}";

		const char* ps_code =
			"varying vec4 color;"
			"void main()"
			"{"
			"gl_FragColor=color;"
			"}";

		/*const char* vs_code =
			"varying vec4 color;"
			"void main()"
			"{"
			"vec4 whiteColor = vec4(1.0, 1.0, 1.0, 1.0);"
			"float depthFactor = (gl_Position.z + 2.0) / 4.0;" // Adjust range to [0.0, 1.0]
			"color=mix(whiteColor, gl_Color, depthFactor);"
			"gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;"
			"}";

		const char* ps_code =
			"varying vec4 color;"
			"void main()"
			"{"
			"gl_FragColor=vec4(1.0, 1.0, 1.0, 1.0);"
			"}";*/

		m_shader.add_program(nya_render::shader::vertex, vs_code);
		m_shader.add_program(nya_render::shader::pixel, ps_code);
	}

	void on_frame(unsigned int dt)
	{
		nya_render::clear(true, true);

		m_rot += dt * 0.05f;
		if (m_rot > 360)
			m_rot = 0;

		nya_math::mat4 mv;
		mv.translate(0, 0, -2.0f);
		// mv.rotate(30.0f, 1.0f, 0.0f, 0.0f);
		mv.rotate(30.0f, 1.0f, 0.0f, 0.0f);
		mv.rotate(m_rot, 1.0f, 1.0f, 1.0f);
		// mv.scale(0.1f, 0.1f, 1.0f);
		mv.scale(0.1f);
		nya_render::set_modelview_matrix(mv);

		m_shader.bind();
		m_vbo.bind();
		m_vbo.draw();
		m_vbo.unbind();
		m_shader.unbind();

		static unsigned int fps_counter = 0, fps_update_timer = 0;
		++fps_counter;
		fps_update_timer += dt;
		if (fps_update_timer > 1000)
		{
			std::ostringstream os;
			os << "test cube " << fps_counter << " fps";
			std::string str = os.str();
			set_title(str.c_str());

			fps_update_timer %= 1000;
			fps_counter = 0;
		}
	}

	void on_resize(unsigned int w, unsigned int h)
	{
		nya_log::log() << "on_resize " << w << " " << h << "\n";

		if (!w || !h)
			return;

		nya_math::mat4 proj;
		proj.perspective(70.0f, float(w) / h, 0.01f, 100.0f);
		nya_render::set_projection_matrix(proj);
	}

	void on_free()
	{
		nya_log::log() << "on_free\n";

		m_vbo.release();
		m_shader.release();
	}

	void on_keyboard(unsigned int key, bool pressed)
	{
		if ((key == nya_system::key_back || key == nya_system::key_escape) && !pressed)
			finish();
	}

	// Handle button click
	void handleButtonClick()
	{
		nya_log::log() << "Button Clicked\n";
	}

	// Handle SDL events
	void handleSDLEvents()
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_BUTTON_RIGHT:
				finish();
				break;
			case SDL_BUTTON_LEFT:
				// Check if the mouse click is within the button area
				if (event.button.button == SDL_BUTTON_LEFT &&
					event.button.x >= buttonRect.x &&
					event.button.x <= buttonRect.x + buttonRect.w &&
					event.button.y >= buttonRect.y &&
					event.button.y <= buttonRect.y + buttonRect.h)
				{
					handleButtonClick();
				}
				break;
				// Add other SDL event handling if needed
			}
		}
	}

public:
	test_cube() : m_rot(0.0f) {}

private:
	nya_render::vbo m_vbo;
	nya_render::shader m_shader;
	float m_rot;
	SDL_Rect buttonRect = { 100, 100, 150, 50 };  // x, y, width, height

};

#ifdef _WIN32
#ifdef WINDOWS_METRO
int main(Platform::Array<Platform::String^>^ args)
#else
// int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
int main(int argc, char** argv)
#endif
#else
int main(int argc, char** argv)
#endif
{

#ifdef __ANDROID__
	nya_log::set_log(new nya_log::android_log("test_cube"));
#endif


	// SDL Initialization
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		nya_log::log() << "SDL initialization failed: " << SDL_GetError() << "\n";
		return 1;
	} else
	{
		nya_log::log() << "SDL initialization success \n";
	}

	test_cube app;
	app.set_title("Loading, please wait...");
	app.start_windowed(100, 100, 640, 480, 0);

	nya_log::log() << "exit success\n";

	return 0;
}