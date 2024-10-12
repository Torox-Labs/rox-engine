//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "app.h"
#include "system.h"
#include "render/render.h"

#include <string>

#ifdef _WIN32
#define _WIN32_WINDOWS 0x0A00

#include <windows.h>

// This Macro (_MSC_VER) is defined by the Microsoft C/C++ compiler.
#if defined(_MSC_VER) && _MSC_VER >= 1900 // VS2015
// This Macro (_WIN32_WINNT) is defined by the Windows SDK.
#if _WIN32_WINNT >= _WIN32_WINNT_WIN10
#include "winapifamily.h"

#if defined(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define WINDOWS_FLUENT
#endif

#endif
#endif

//TODO: Update This To Use Windows FLUENT API For Windows 10 & 11
#ifdef WINDOWS_FLUENT
#include <wrl/client.h>
#include <ppl.h>
#include <d3d11_1.h>

#include <winrt/Windows.UI.Xaml.Controls.h>

using namespace winrt;
using namespace concurrency;

namespace
{

    class shared_app
    {
    public:
        void start_windowed(int x, int y, unsigned int w, unsigned int h, int antialiasing, nya_system::app& app)
        {
            m_width = w;
            m_height = h;
            init_window(x, y);
            init_direct3d();
            run();
        }

        void start_fullscreen(unsigned int w, unsigned int h, int aa, nya_system::app& app)
        {
            start_windowed(0, 0, w, h, aa, app);
        }

        void finish(nya_system::app& app)
        {
            if(m_swapChain)
            {
				m_swapChain->SetFullscreenState(FALSE, nullptr);
            }
        }

        void set_title(const char* title)
        {
			SetWindowText(m_hwnd, title);
        }

        void set_virtual_keyboard(int type)
        {
            // Set virtual keyboard based on Fluent requirements (optional implementation)
        }

    public:
        static shared_app& get_app()
        {
            static shared_app app;
            return app;
        }

    private:
        void init_window(int x, int y)
        {
	        WNDCLASSEX wcex = {};
	        wcex.cbSize = sizeof(WNDCLASSEX);
	        wcex.style = CS_HREDRAW | CS_VREDRAW;
	        wcex.lpfnWndProc = DefWindowProc;
	        wcex.hInstance = GetModuleHandle(nullptr);
	        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	        wcex.lpszClassName = "Direct3DWindowClass";
	        RegisterClassEx(&wcex);


	        m_hwnd = CreateWindow("Direct3DWindowClass", "Direct3D with Fluent UI", WS_OVERLAPPEDWINDOW, x, y,
	                              m_width, m_height, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	        ShowWindow(m_hwnd, SW_SHOW);
        }

        void init_direct3d()
        {
	        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
			creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	        D3D_FEATURE_LEVEL featureLevels[] = {
		        D3D_FEATURE_LEVEL_12_1,
		        D3D_FEATURE_LEVEL_12_0,
		        D3D_FEATURE_LEVEL_11_1,
		        D3D_FEATURE_LEVEL_11_0,
		        D3D_FEATURE_LEVEL_10_1,
		        D3D_FEATURE_LEVEL_10_0,
	        };

			com_ptr<ID3D11Device> device;
			com_ptr<ID3D11DeviceContext> context;
			D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, creationFlags, featureLevels, ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION, device.put(), &m_featureLevel, context.put());

            device.as(m_device);
			context.as(m_context);

            com_ptr<IDXGIDevice> dxgiDevice;
			m_device.as(dxgiDevice);

            com_ptr <IDXGIAdapter > adapter;
			dxgiDevice->GetAdapter(adapter.put());

			com_ptr<IDXGIFactory2 > factory;
            adapter->GetParent(__uuidof(IDXGIFactory), factory.put_void());

            DXGI_SWAP_CHAIN_DESC sd = {};
			sd.BufferCount = 1;
			sd.BufferDesc.Width = m_width;
			sd.BufferDesc.Height = m_height;
			sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            sd.OutputWindow = m_hwnd;
            sd.SampleDesc.Count = 1;
            sd.Windowed = TRUE;

            factory->CreateSwapChain(m_device.get(), &sd, m_swapChain.put());
            create_render_target();
        }

        void create_render_target()
        {
            com_ptr<ID3D11Texture2D> backBuffer;
            m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), backBuffer.put_void());
            m_device->CreateRenderTargetView(backBuffer.get(), nullptr, m_renderTargetView.put());
            m_context->OMSetRenderTargets(1, m_renderTargetView.put(), nullptr);
        }

        void run()
        {
            MSG msg = {};
            while(WM_QUIT != msg.message)
            {
	            if(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	            {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
	            }
                else
                {
                    render();
                }
            }
        }

        void render()
        {
            const float clearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
            m_context->ClearRenderTargetView(m_renderTargetView.get(), clearColor);
            m_swapChain->Present(1,0);
        }

    private:
        unsigned int m_width;
        unsigned int m_height;
        HWND m_hwnd;
        com_ptr<ID3D11Device> m_device;
		com_ptr<ID3D11DeviceContext> m_context;
        com_ptr<IDXGISwapChain> m_swapChain;
        com_ptr<ID3D11RenderTargetView> m_renderTargetView;
		D3D_FEATURE_LEVEL m_featureLevel;
    };

}

#else
// ... other code
#endif
#elif __ANDROID__
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <android/keycodes.h>
#include "resources/apk_resources_provider.h"
#include <android/asset_manager_jni.h>
#include <EGL/egl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>

int main(int argc,const char *argv[]);

namespace { bool non_native_exit=false; }

class native_process
{
public:
    static native_process &get() { static native_process p; return p; }

public:
    native_process(): m_started(false) { pthread_mutex_init(&m_mutex,0); }
    ~native_process() { pthread_mutex_destroy(&m_mutex); }

    void start() { if(m_started) return; pthread_create(&m_thread,0,thread_callback,0); m_started=true; }
    void stop() { pthread_join(m_thread,0); m_started=false; }

    void lock() { pthread_mutex_lock(&m_mutex); }
    void unlock() { pthread_mutex_unlock(&m_mutex); }

    void sleep(unsigned int msec) { usleep(msec); }

private:
    static void* thread_callback(void *) { main(0,0); if(!non_native_exit) exit(0); return 0; }

private:
    pthread_t m_thread;
    pthread_mutex_t m_mutex;
    bool m_started;
};

namespace
{
    ANativeWindow *window=0;
    bool native_paused=false;
    bool should_exit=false;
    bool suspend_ready=false;

    struct input_event { int x,y,id; bool pressed,btn; };
    std::vector<input_event> input_events;
    struct input_key { unsigned int code; bool pressed; unsigned int unicode_char; bool autorepeat; };
    std::vector<input_key> input_keys;
}

static unsigned int get_x11_key(int key)
{
    if(key>=AKEYCODE_A  && key<=AKEYCODE_Z)
        return nya_system::key_a+key-AKEYCODE_A;

    if(key>=7 && key<=16)
        return nya_system::key_0+key-7;
    if(key>=AKEYCODE_0 && key<= AKEYCODE_9)
        return nya_system::key_0+key-AKEYCODE_0;

    if(key>=131 && key<=142)
        return nya_system::key_f1+key-131;
    if(key>=AKEYCODE_F1 && key<=AKEYCODE_F12)
        return nya_system::key_f1+key-AKEYCODE_F1;

    switch(key)
    {
            case AKEYCODE_BACK: return nya_system::key_back;

            case AKEYCODE_DPAD_UP: return nya_system::key_up;
            case AKEYCODE_DPAD_DOWN: return nya_system::key_down;
            case AKEYCODE_DPAD_LEFT: return nya_system::key_left;
            case AKEYCODE_DPAD_RIGHT: return nya_system::key_right;
            case AKEYCODE_DPAD_CENTER: return nya_system::key_return; //dpad center

            case AKEYCODE_TAB: return nya_system::key_tab;
            case AKEYCODE_SPACE: return nya_system::key_space;
            case AKEYCODE_ENTER: return nya_system::key_return;
            case AKEYCODE_DEL: return nya_system::key_backspace;
            case AKEYCODE_ESCAPE: return nya_system::key_escape;
            case AKEYCODE_FORWARD_DEL: return nya_system::key_delete;

            case AKEYCODE_MOVE_HOME: return nya_system::key_home;
            case AKEYCODE_MOVE_END: return nya_system::key_end;
            case AKEYCODE_INSERT: return nya_system::key_insert;
    }

    return 0;
}

namespace nya_system { void set_android_user_path(const char *path); }

namespace
{
    JavaVM *java_vm;
    jclass java_class;

    JNIEnv *java_get_env()
    {
        JNIEnv *env;
        int r=java_vm->GetEnv((void **)&env,JNI_VERSION_1_6);
        if(r<0 && java_vm->AttachCurrentThread(&env,NULL)<0)
            return 0;
        return env;
    }
}

extern "C"
{
    JNIEXPORT void JNICALL Java_nya_native_1activity_native_1spawn_1main(JNIEnv *env,jobject obj)
    {
        window=0;
        native_paused=false;
        should_exit=false;
        suspend_ready=false;
        non_native_exit=false;
		env->GetJavaVM(&java_vm);
		jclass class_act=env->FindClass("nya/native_activity");
		java_class=jclass(env->NewGlobalRef(class_act));
		env->DeleteLocalRef(class_act);

        native_process::get().start();
    }

    JNIEXPORT void JNICALL Java_nya_native_1activity_native_1pause(JNIEnv *env,jobject obj)
    {
        native_process::get().lock();
        suspend_ready=false;
        native_paused=true;
        native_process::get().unlock();

        bool not_ready=true;
        while(not_ready)
        {
            native_process::get().lock();
            if(suspend_ready)
                not_ready=false;
            native_process::get().unlock();

            if(not_ready)
                native_process::get().sleep(10);
        }
    }

    JNIEXPORT void JNICALL Java_nya_native_1activity_native_1resume(JNIEnv *env,jobject obj)
    {
        native_process::get().lock();
        native_paused=false;
        native_process::get().unlock();
    }

    JNIEXPORT void JNICALL Java_nya_native_1activity_native_1touch(JNIEnv *env,jobject obj,int x,int y,int id,bool pressed,bool btn)
    {
        native_process::get().lock();
        input_event e; e.x=x; e.y=y; e.id=id; e.pressed=pressed; e.btn=btn;
        input_events.push_back(e);
        native_process::get().unlock();
    }

    JNIEXPORT bool JNICALL Java_nya_native_1activity_native_1key(JNIEnv *env,jobject obj,int code,bool pressed, int unicode_char, bool autorepeat)
    {
        const unsigned int x11key=get_x11_key(code);
        native_process::get().lock();
        input_key k; k.code=x11key; k.pressed=pressed; k.unicode_char=unicode_char; k.autorepeat=autorepeat;
        input_keys.push_back(k);
        native_process::get().unlock();
        return true;
    }

    JNIEXPORT void JNICALL Java_nya_native_1activity_native_1exit(JNIEnv *env,jobject obj)
    {
        non_native_exit=true;
        native_process::get().lock();
        should_exit=true;
        native_process::get().unlock();
        native_process::get().stop();
    }

    JNIEXPORT void JNICALL Java_nya_native_1activity_native_1set_1surface(JNIEnv *env,jobject obj,jobject surface)
    {
        if(surface)
        {
            native_process::get().lock();
            if(window)
                ANativeWindow_release(window);
            window=ANativeWindow_fromSurface(env,surface);
            native_process::get().unlock();
        }
        else
        {
            native_process::get().lock();
            if(window)
                ANativeWindow_release(window);
            window=0;
            native_process::get().unlock();
        }
    }

    JNIEXPORT void JNICALL Java_nya_native_1activity_native_1set_1asset_1mgr(JNIEnv *env,jobject obj,jobject asset_mgr)
    {
        nya_resources::apk_resources_provider::set_asset_manager(AAssetManager_fromJava(env,asset_mgr));
    }

    JNIEXPORT void JNICALL Java_nya_native_1activity_native_1set_1user_1path(JNIEnv *env,jobject obj,jstring path)
    {
        const char *s=env->GetStringUTFChars(path,JNI_FALSE);
        nya_system::set_android_user_path(s);
        env->ReleaseStringUTFChars(path,s);
    }
};

class egl_renderer
{
public:
    bool init(ANativeWindow *window)
    {
        if(!window)
            return false;

        if(m_context!=EGL_NO_CONTEXT)
            return true;

        if(m_display==EGL_NO_DISPLAY)
            m_display=eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if(m_display==EGL_NO_DISPLAY)
            return false;

        if(!eglInitialize(m_display,NULL,NULL))
            return false;

        EGLint RGBX_8888_ATTRIBS[]=
        {
            EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
            EGL_BLUE_SIZE,8,EGL_GREEN_SIZE,8,EGL_RED_SIZE,8,
            EGL_DEPTH_SIZE,24,
            EGL_NONE
        };

        EGLint RGB_565_ATTRIBS[]=
        {
            EGL_RENDERABLE_TYPE,EGL_OPENGL_ES2_BIT,
            EGL_SURFACE_TYPE,EGL_WINDOW_BIT,
            EGL_BLUE_SIZE,5,EGL_GREEN_SIZE,6,EGL_RED_SIZE,5,
            EGL_DEPTH_SIZE,24,
            EGL_NONE
        };

        EGLint* attrib_list;

        int window_format=ANativeWindow_getFormat(window);
        if(window_format==WINDOW_FORMAT_RGBA_8888 || window_format==WINDOW_FORMAT_RGBX_8888)
            attrib_list=RGBX_8888_ATTRIBS;
        else
            attrib_list=RGB_565_ATTRIBS;

        EGLConfig config;
        EGLint num_configs=0;
        if(!eglChooseConfig(m_display,attrib_list,&config,1,&num_configs) || !num_configs)
        {
            set_attrib(attrib_list,EGL_DEPTH_SIZE,16);
            if(!eglChooseConfig(m_display,attrib_list,&config,1,&num_configs) || !num_configs)
            {
                nya_system::log()<<"ERROR: unable to choose egl config\n";
                return false;
            }
        }

        EGLint format;
        if(!eglGetConfigAttrib(m_display,config,EGL_NATIVE_VISUAL_ID,&format))
        {
            nya_system::log()<<"ERROR: unable to get egl config attributes\n";
            return false;
        }

        if(ANativeWindow_setBuffersGeometry(window,0,0,format)!=0)
        {
            nya_system::log()<<"ERROR: unable to set egl buffers geometry\n";
            return false;
        }

        m_surface=eglCreateWindowSurface(m_display,config,window,NULL);
        if(m_surface==EGL_NO_SURFACE)
        {
            nya_system::log()<<"ERROR: unable to create egl surface\n";
            return false;
        }

        if(m_saved_context==EGL_NO_CONTEXT)
        {
            EGLint context_attribs[]={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
            m_context=eglCreateContext(m_display,config,EGL_NO_CONTEXT,context_attribs);
            if(m_context==EGL_NO_CONTEXT)
            {
                nya_system::log()<<"ERROR: unable to create egl context\n";
                return false;
            }
        }
        else
            m_context=m_saved_context;

        if(!eglMakeCurrent(m_display,m_surface,m_surface,m_context))
        {
            nya_system::log()<<"ERROR: unable to make egl context current\n";
            return false;
        }

        m_saved_context=EGL_NO_CONTEXT;

        if(!querry_size())
            return false;

        return true;
    }

    void end_frame() { eglSwapBuffers(m_display,m_surface); }

    bool querry_size()
    {
        if(!m_display || !m_surface)
            return false;

        if(!eglQuerySurface(m_display,m_surface,EGL_WIDTH,&m_width))
        {
            nya_system::log()<<"ERROR: unable to querry egl surface width\n";
            return false;
        }

        if(!eglQuerySurface(m_display,m_surface,EGL_HEIGHT,&m_height))
        {
            nya_system::log()<<"ERROR: unable to querry egl surface height\n";
            return false;
        }

        return true;
    }

    void suspend()
    {
        if(m_context==EGL_NO_CONTEXT)
            return;

        m_saved_context=m_context;
        m_context=EGL_NO_CONTEXT;
        if(!eglMakeCurrent(m_display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT))
            return;

        if(m_surface!=EGL_NO_SURFACE)
            eglDestroySurface(m_display,m_surface);
        m_surface=EGL_NO_SURFACE;
    }

    void destroy()
    {
        if(m_display!=EGL_NO_DISPLAY)
        {
            eglMakeCurrent(m_display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
            if(m_context!=EGL_NO_CONTEXT)
                eglDestroyContext(m_display,m_context);
            if(m_surface!=EGL_NO_SURFACE)
                eglDestroySurface(m_display,m_surface);
            eglTerminate(m_display);
        }

        m_display=EGL_NO_DISPLAY;
        m_context=EGL_NO_CONTEXT;
        m_surface=EGL_NO_SURFACE;
    }

    int get_width() const { return (int)m_width; }
    int get_height() const { return (int)m_height; }

private:
    bool set_attrib(EGLint *list,EGLint param,EGLint value)
    {
        while(*list!=EGL_NONE)
        {
            if(*list!=param)
            {
                ++list;
                continue;
            }

            *(list+1)=value;
            return true;
        }

        return false;
    }

public:
    egl_renderer(): m_display(EGL_NO_DISPLAY),m_surface(EGL_NO_SURFACE),
                    m_context(EGL_NO_CONTEXT),m_saved_context(EGL_NO_CONTEXT),
                    m_width(0),m_height(0) {}
private:
    EGLDisplay  m_display;
    EGLSurface  m_surface;
    EGLContext m_context;
    EGLint m_width,m_height;
    EGLContext m_saved_context;
};

class shared_app
{
public:
    void start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
    {
        bool initialised=false;
        bool paused=false;
        bool need_restore=false;
        ANativeWindow *last_window=0;

        while(true)
        {
            native_process::get().lock();

            if(should_exit)
            {
                native_process::get().unlock();
                break;
            }

            if(initialised)
            {
                if(need_restore)
                {
                    if(window)
                    {
                        m_renderer.init(window);
                        nya_render::apply_state(true);
                        app.on_restore();
                        m_time=nya_system::get_time();
                        need_restore=false;
                        last_window=window;
                    }
                }
                else if(!paused && last_window!=window)
                {
                    const int w=m_renderer.get_width(),h=m_renderer.get_height();
                    m_renderer.querry_size();
                    if(w!=m_renderer.get_width() || h!=m_renderer.get_height())
                    {
                        nya_render::set_viewport(0,0,m_renderer.get_width(),m_renderer.get_height());
                        app.on_resize(m_renderer.get_width(),m_renderer.get_height());
                    }
                    last_window=window;
                }
            }
            else if(window)
            {
                m_renderer.init(window);
                nya_render::set_viewport(0,0,m_renderer.get_width(),m_renderer.get_height());
                app.on_resize(m_renderer.get_width(),m_renderer.get_height());
                if(app.on_splash())
                    m_renderer.end_frame();

                app.on_init();
                m_time=nya_system::get_time();
                initialised=true;
                last_window=window;
            }

            if(paused!=native_paused)
            {
                paused=native_paused;
                if(native_paused)
                {
                    app.on_suspend();
                    m_renderer.suspend();
                    suspend_ready=true;
                }
                else
                    need_restore=true;
            }

            for(size_t i=0;i<input_keys.size();++i)
            {
                if(input_keys[i].code>0)
                    app.on_keyboard(input_keys[i].code,input_keys[i].pressed);
                if(input_keys[i].unicode_char>0)
                    app.on_charcode(input_keys[i].unicode_char,input_keys[i].pressed,input_keys[i].autorepeat);
            }
            input_keys.clear();

            for(size_t i=0;i<input_events.size();++i)
            {
                input_event &e=input_events[i];
                int y=m_renderer.get_height()-e.y;
                app.on_touch(e.x,y,e.id,e.pressed);
                if(e.id!=0)
                    continue;

                app.on_mouse_move(e.x,y);
                if(e.btn)
                    app.on_mouse_button(nya_system::mouse_left,e.pressed);
            }
            input_events.clear();

            native_process::get().unlock();

            if(paused)
            {
                native_process::get().sleep(500);
                continue;
            }

            if(!initialised || need_restore)
            {
                native_process::get().sleep(10);
                continue;
            }

            const unsigned long time=nya_system::get_time();
            const unsigned int dt=(unsigned int)(time-m_time);
            m_time=time;

            app.on_frame(dt);
            m_renderer.end_frame();
        }

        if(!initialised)
            return;

        app.on_free();
        m_renderer.destroy();
    }

    void start_fullscreen(unsigned int w,unsigned int h,int aa,nya_system::app &app)
    {
        start_windowed(0,0,w,h,aa,app);
    }

    void finish(nya_system::app &app)
    {
        should_exit=true;
    }

    void set_title(const char *title) {}

	void set_virtual_keyboard(int type) 
	{
	    JNIEnv* env=java_get_env();
        jmethodID setVirtualKeyboard=env->GetStaticMethodID(java_class,"setVirtualKeyboard","(I)V");
        env->CallStaticVoidMethod(java_class,setVirtualKeyboard,type);
	}

public:
    static shared_app &get_app()
    {
        static shared_app app;
        return app;
    }

private:
    unsigned long m_time;
    egl_renderer m_renderer;
};

#elif defined __APPLE__ //implemented in app.mm

#elif defined EMSCRIPTEN

#include <emscripten/emscripten.h>
#include <GLFW/glfw3.h>

namespace { bool is_fs_ready=false; }
extern "C" __attribute__((used)) void emscripten_on_fs_ready() { is_fs_ready=true;  }

namespace
{

class shared_app
{
public:
    static shared_app &get_app()
    {
        static shared_app app;
        return app;
    }

public:
    void start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
    {
        if(m_window)
            return;

        m_app=&app;
        m_width=w;
        m_height=h;
        m_need_init=true;

        EM_ASM(
            FS.mkdir('/.nya');
            FS.mount(IDBFS,{},'/.nya');
            FS.syncfs(true, function (err) { ccall('emscripten_on_fs_ready','v'); });
        );

        emscripten_set_main_loop(&main_loop,0,NULL);
        emscripten_exit_with_live_runtime();
    }

    void start_fullscreen(unsigned int w,unsigned int h,int aa,nya_system::app &app)
    {
        start_windowed(0,0,w,h,aa,app);
    }

    void finish(nya_system::app &app) {}
    void set_title(const char *title) {}
	void set_virtual_keyboard(int type) {}

private:
    void do_frame()
    {
        if(m_need_init)
        {
            if(!is_fs_ready)
            {
                usleep(20*1000);
                return;
            }

            glfwInit();
            m_window=glfwCreateWindow(m_width,m_height,"Nya engine",NULL,NULL);

            glfwSetCursorPosCallback(m_window,cursor_pos_callback);
            glfwSetMouseButtonCallback(m_window,mouse_button_callback);
            glfwSetKeyCallback(m_window,key_callback);

            nya_render::set_viewport(0,0,m_width,m_height);
            m_app->on_resize(m_width,m_height);
            m_app->on_splash();
            m_app->on_init();
            m_time=nya_system::get_time();

            m_need_init=false;
        }

        glfwPollEvents();

        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        if(m_width!=width || m_height!=height)
        {
            nya_render::set_viewport(0,0,width,height);
            m_app->on_resize(width,height);
            m_width=width;
            m_height=height;
        }

        const unsigned long time=nya_system::get_time();
        const unsigned int dt=(unsigned int)(time-m_time);
        m_time=time;
        m_app->on_frame(dt);
        glfwSwapBuffers(m_window);
    }

    static void cursor_pos_callback(GLFWwindow* window,double xpos,double ypos)
    {
        get_app().m_app->on_mouse_move(int(xpos),get_app().m_height-int(ypos));
    }

    static void mouse_button_callback(GLFWwindow* window,int button,int action,int mods)
    {
        get_app().m_app->on_mouse_button(button==GLFW_MOUSE_BUTTON_RIGHT?
                                         nya_system::mouse_right:nya_system::mouse_left,action==GLFW_PRESS);
    }

    static void key_callback(GLFWwindow* window,int key,int scancode,int action,int mods)
    {
        const int x11key=get_x11_key(key);
        if(!x11key)
            return;

        if(action==GLFW_PRESS)
            get_app().m_app->on_keyboard(key,true);
        else if(action==GLFW_RELEASE)
            get_app().m_app->on_keyboard(key,false);
    }

    static unsigned int get_x11_key(unsigned int key)
    {
        if(key>=GLFW_KEY_A && key<=GLFW_KEY_Z)
            return nya_system::key_a+key-GLFW_KEY_A;

        if(key>=GLFW_KEY_0 && key<=GLFW_KEY_9)
            return nya_system::key_0+key-GLFW_KEY_0;

        if(key>=GLFW_KEY_F1 && key<=GLFW_KEY_F12)
            return nya_system::key_f1+key-GLFW_KEY_F1;

        switch(key)
        {
            case GLFW_KEY_LEFT_SHIFT: return nya_system::key_shift;
            case GLFW_KEY_RIGHT_SHIFT: return nya_system::key_shift;
            case GLFW_KEY_LEFT_CONTROL: return nya_system::key_control;
            case GLFW_KEY_RIGHT_CONTROL: return nya_system::key_control;
            case GLFW_KEY_LEFT_ALT: return nya_system::key_alt;
            case GLFW_KEY_RIGHT_ALT: return nya_system::key_alt;

            case GLFW_KEY_ESCAPE: return nya_system::key_escape;
            case GLFW_KEY_SPACE: return nya_system::key_space;
            case GLFW_KEY_ENTER: return nya_system::key_return;
            case GLFW_KEY_TAB: return nya_system::key_tab;

            case GLFW_KEY_END: return nya_system::key_end;
            case GLFW_KEY_HOME: return nya_system::key_home;
            case GLFW_KEY_INSERT: return nya_system::key_insert;
            case GLFW_KEY_DELETE: return nya_system::key_delete;

            case GLFW_KEY_UP: return nya_system::key_up;
            case GLFW_KEY_DOWN: return nya_system::key_down;
            case GLFW_KEY_LEFT: return nya_system::key_left;
            case GLFW_KEY_RIGHT: return nya_system::key_right;
        }
        
        return 0;
    }

private:
    static void main_loop() { shared_app::get_app().do_frame(); }

public:
    shared_app(): m_window(0),m_time(0),m_width(0),m_height(0) {}

private:
    GLFWwindow *m_window;
    unsigned long m_time;
    nya_system::app *m_app;
    bool m_need_init;
    int m_width,m_height;
};

}
#else

//  fullscreen:
//#include <X11/Xlib.h>
//#include <X11/Xatom.h>
//#include <X11/extensions/xf86vmode.h> //libxxf86vm-dev libXxf86vm.a

#include <GL/glx.h>
#include <GL/gl.h>
#include <X11/X.h>
#include <X11/keysym.h>

namespace
{

class shared_app
{
public:
    void start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
    {
        if(m_dpy)
            return;

        m_dpy=XOpenDisplay(NULL);
        if(!m_dpy)
        {
            nya_system::log()<<"unable to open x display\n";
            return;
        }

        int dummy;
        if(!glXQueryExtension(m_dpy,&dummy,&dummy))
        {
            nya_system::log()<<"unable to querry glx extension\n";
            return;
        }

        static int dbl_buf[]={GLX_RGBA,GLX_DEPTH_SIZE,24,GLX_DOUBLEBUFFER,None};

        static int dbl_buf_aniso[]={GLX_RGBA,GLX_DEPTH_SIZE,24,GLX_DOUBLEBUFFER,
                        GLX_SAMPLE_BUFFERS_ARB,1,GLX_SAMPLES,antialiasing,None};

        XVisualInfo *vi=0;
        if(antialiasing>1)
        {
            vi=glXChooseVisual(m_dpy,DefaultScreen(m_dpy),dbl_buf_aniso);
            if(!vi)
            {
                nya_system::log()<<"unable to set antialising\n";
                antialiasing=0;
            }
        }

        if(antialiasing<=1)
            vi=glXChooseVisual(m_dpy,DefaultScreen(m_dpy),dbl_buf);

        if(!vi)
        {
            nya_system::log()<<"unable to choose glx visual\n";
            return;
        }

        if(vi->c_class!=TrueColor)
        {
            nya_system::log()<<"device does not support TrueColor\n";
            return;
        }

        m_cx=glXCreateContext(m_dpy,vi,None,GL_TRUE);
        if(!m_cx)
        {
            nya_system::log()<<"unable to ceate glx context\n";
            return;
        }

        Colormap cmap=XCreateColormap(m_dpy,RootWindow(m_dpy,vi->screen),vi->visual,AllocNone);

        XSetWindowAttributes swa;
        swa.colormap=cmap;
        swa.border_pixel=0;
        swa.event_mask=KeyPressMask| ExposureMask|ButtonPressMask|
                       StructureNotifyMask|ButtonReleaseMask | PointerMotionMask;

        m_win=XCreateWindow(m_dpy,RootWindow(m_dpy,vi->screen),x,y,
                  w,h,0,vi->depth,InputOutput,vi->visual,
                  CWBorderPixel|CWColormap|CWEventMask,&swa);

        XSetStandardProperties(m_dpy,m_win,m_title.c_str(),m_title.c_str(),None,0,0,NULL);
        glXMakeCurrent(m_dpy,m_win,m_cx);
        XMapWindow(m_dpy,m_win);

        if(antialiasing>1)
            glEnable(GL_MULTISAMPLE_ARB);

        nya_render::set_viewport(0,0,w,h);
        app.on_resize(w,h);
        m_time=nya_system::get_time();

        if(app.on_splash())
            glXSwapBuffers(m_dpy,m_win);

        app.on_init();

        m_time=nya_system::get_time();

        XEvent event;
        while(true)
        {
            while(XPending(m_dpy))
            {
                XNextEvent(m_dpy, &event);
                switch (event.type)
                {
                    case ConfigureNotify:
                    {
                        if(w!=event.xconfigure.width || h!=event.xconfigure.height)
                        {
                            w=event.xconfigure.width;
                            h=event.xconfigure.height;
                            nya_render::set_viewport(0, 0, w, h);
                            app.on_resize(w,h);
                        }
                    }
                    break;

                    case MotionNotify:
                        app.on_mouse_move(event.xmotion.x,h-event.xmotion.y);
                    break;

                    case ButtonPress:
                    {
                        const int scroll_modifier=16;

                        switch (event.xbutton.button)
                        {
                            case 1:
                                app.on_mouse_button(nya_system::mouse_left,true);
                            break;

                            case 2:
                                app.on_mouse_button(nya_system::mouse_middle,true);
                            break;

                            case 3:
                                app.on_mouse_button(nya_system::mouse_right,true);
                            break;

                            case 4:
                                app.on_mouse_scroll(0,scroll_modifier);
                            break;

                            case 5:
                                app.on_mouse_scroll(0,-scroll_modifier);
                            break;

                            case 6:
                                app.on_mouse_scroll(scroll_modifier,0);
                            break;

                            case 7:
                                app.on_mouse_scroll(-scroll_modifier,0);
                            break;
                        }
                    }
                    break;

                    case ButtonRelease:
                        switch (event.xbutton.button)
                        {
                            case 1:
                                app.on_mouse_button(nya_system::mouse_left,false);
                            break;

                            case 2:
                                app.on_mouse_button(nya_system::mouse_middle,false);
                            break;

                            case 3:
                                app.on_mouse_button(nya_system::mouse_right,false);
                            break;
                        }
                    break;
                };
            }

            const unsigned long time=nya_system::get_time();
            const unsigned int dt=(unsigned int)(time-m_time);
            m_time=time;

            app.on_frame(dt);

            glXSwapBuffers(m_dpy,m_win);
        }

        finish(app);
    }

    void start_fullscreen(unsigned int w,unsigned int h,int aa,nya_system::app &app)
    {
        //ToDo

        start_windowed(0,0,w,h,aa,app);
    }

    void finish(nya_system::app &app)
    {
        if(!m_dpy || !m_cx)
            return;

        app.on_free();

        if(!glXMakeCurrent(m_dpy,None,NULL))
        {
            nya_system::log()<<"Could not release drawing context.\n";
            return;
        }

        glXDestroyContext(m_dpy,m_cx);
        m_cx=0;
        m_dpy=0;
    }

    void set_title(const char *title)
    {
        if(!title)
        {
            m_title.clear();
            return;
        }

        m_title.assign(title);

        if(!m_dpy || !m_win)
            return;

        XSetStandardProperties(m_dpy,m_win,title,title,None,0,0,NULL);
    }
	
	void set_virtual_keyboard(int type) {}

public:
    static shared_app &get_app()
    {
        static shared_app app;
        return app;
    }

public:
    shared_app():m_dpy(0),m_win(0),m_title("Nya engine"),m_time(0) {}

private:
    Display *m_dpy;
    Window m_win;
    GLXContext m_cx;
    std::string m_title;
    unsigned long m_time;
};

}

#endif

#ifndef __APPLE__ //implemented in app.mm

namespace nya_system
{
	void app::start_windowed(int x, int y, unsigned int w, unsigned int h, int antialiasing)
	{
		shared_app::get_app().start_windowed(x, y, w, h, antialiasing, *this);
	}

	void app::start_fullscreen(unsigned int w, unsigned int h, int aa)
	{
		shared_app::get_app().start_fullscreen(w, h, aa, *this);
	}

	void app::set_title(const char* title)
	{
		shared_app::get_app().set_title(title);
	}

	void app::set_virtual_keyboard(virtual_keyboard_type type)
	{
		shared_app::get_app().set_virtual_keyboard(type);
	}

	void app::set_mouse_pos(int x, int y)
	{
	}

	void app::finish()
	{
		shared_app::get_app().finish(*this);
	}
}

#endif
