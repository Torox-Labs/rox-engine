//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "app.h"
#include "system.h"
#include "render/render.h"

#include <string>

#ifdef _WIN32
  #define _WIN32_WINDOWS 0x501

  #include <windows.h>
  #if defined(_MSC_VER) && _MSC_VER >= 1700
        #if _WIN32_WINNT >= _WIN32_WINNT_WIN8 && !_USING_V110_SDK71_
            #include "winapifamily.h"
            #if defined(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            #elif defined(WINAPI_PARTITION_PHONE) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_PHONE)
                #define WINDOWS_PHONE8
                #define WINDOWS_METRO
            #elif defined(WINAPI_PARTITION_APP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
                #define WINDOWS_METRO
            #endif
        #endif
  #endif

  #ifdef WINDOWS_METRO
    #include <wrl/client.h>
    #include <ppl.h>
    #include <agile.h>
    #include <d3d11_1.h>
    #include <math.h>

    using namespace Windows::ApplicationModel;
    using namespace Windows::ApplicationModel::Core;
    using namespace Windows::ApplicationModel::Activation;
    using namespace Windows::UI::Core;
    using namespace Windows::System;
    using namespace Windows::Foundation;
    using namespace Windows::Graphics::Display;
    using namespace concurrency;

namespace
{

ref class PhoneDirect3DApp sealed : public Windows::ApplicationModel::Core::IFrameworkView
{
    friend ref class Direct3DApplicationSource;
public:
    //IFrameworkView.
    virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView)
    {
        applicationView->Activated +=
            ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &PhoneDirect3DApp::OnActivated);

        CoreApplication::Suspending +=
            ref new EventHandler<SuspendingEventArgs^>(this, &PhoneDirect3DApp::OnSuspending);

        CoreApplication::Resuming +=
            ref new EventHandler<Platform::Object^>(this, &PhoneDirect3DApp::OnResuming);
    }

    virtual void SetWindow(Windows::UI::Core::CoreWindow^ window)
    {
        window->VisibilityChanged +=
            ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &PhoneDirect3DApp::OnVisibilityChanged);

        window->Closed +=
            ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &PhoneDirect3DApp::OnWindowClosed);

        window->PointerPressed +=
            ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &PhoneDirect3DApp::OnPointerPressed);

        window->PointerMoved +=
            ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &PhoneDirect3DApp::OnPointerMoved);

        window->PointerReleased +=
            ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &PhoneDirect3DApp::OnPointerReleased);

        m_window=window;

        CreateContext();
        CreateTargets();

        m_height=int(m_renderTargetSize.Height);
        m_app.on_resize((unsigned int)m_renderTargetSize.Width,(unsigned int)m_renderTargetSize.Height);
        m_time=nya_system::get_time();
        if(m_app.on_splash())
            m_swap_chain->Present(1, 0);
        m_app.on_init();

        m_time=nya_system::get_time();
    }

    virtual void Load(Platform::String^ entryPoint)
    {
    }

    virtual void Run()
    {
        while (!m_windowClosed)
        {
            if (m_windowVisible)
            {
                unsigned long time=nya_system::get_time();
                unsigned int dt=(unsigned)(time-m_time);
                m_time=time;

                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

                m_app.on_frame(dt);

                if(m_swap_chain->Present(1, 0)==DXGI_ERROR_DEVICE_REMOVED)
                {
                    //ToDo
                }
            }
            else
                CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
        }
    }
    virtual void Uninitialize()
    {
    }

protected:
    void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args)
    {
        CoreWindow::GetForCurrentThread()->Activate();
    }

    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args)
    {
        SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();
        //release resources for suspending

        /*
        create_task([this, deferral]()
        {
            deferral->Complete();
        });
        */
    }

    void OnResuming(Platform::Object^ sender, Platform::Object^ args)
    {
    }

    void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args)
    {
        m_windowClosed=true;
    }

    void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args)
    {
        m_windowVisible=args->Visible;
    }

    float get_resolution_scale()
    {
         //ToDo: DeviceExtendedProperties.GetValue("PhysicalScreenResolution");

        const int rs = (int)Windows::Graphics::Display::DisplayProperties::ResolutionScale;
        return (rs>0)?rs/100.0f:1.0f;
    }

    void OnPointerPressed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
    {
        static float rs = get_resolution_scale();
        m_app.on_mouse_move(int(args->CurrentPoint->Position.X*rs),m_height-int(args->CurrentPoint->Position.Y*rs));
        m_app.on_mouse_button(nya_system::mouse_left,true);
    }

    void OnPointerMoved(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
    {
        static float rs = get_resolution_scale();
        m_app.on_mouse_move(int(args->CurrentPoint->Position.X*rs),m_height-int(args->CurrentPoint->Position.Y*rs));
    }

    void OnPointerReleased(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::PointerEventArgs^ args)
    {
        m_app.on_mouse_button(nya_system::mouse_left,false);
    }

    void CreateContext()
    {
        UINT creationFlags=D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    #if defined(_DEBUG)
        creationFlags|=D3D11_CREATE_DEVICE_DEBUG;
    #endif
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3
        };


        D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,creationFlags,featureLevels,
                          ARRAYSIZE(featureLevels),D3D11_SDK_VERSION,&m_device,&m_featureLevel,&m_context);

        nya_render::set_device(m_device);
        nya_render::set_context(m_context);
        nya_render::cull_face::disable();
    }

    void CreateTargets()
    {
        m_windowBounds = m_window->Bounds;

        m_renderTargetSize.Width = floorf(m_windowBounds.Width * get_resolution_scale());
        m_renderTargetSize.Height = floorf(m_windowBounds.Height * get_resolution_scale());

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
        swapChainDesc.Width=static_cast<UINT>(m_renderTargetSize.Width);
        swapChainDesc.Height=static_cast<UINT>(m_renderTargetSize.Height);
        swapChainDesc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;
        swapChainDesc.Stereo=false;
        swapChainDesc.SampleDesc.Count=1;
        swapChainDesc.SampleDesc.Quality=0;
        swapChainDesc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount=1;
        swapChainDesc.Scaling=DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
        swapChainDesc.Flags=0;

        IDXGIDevice1 *device;
        if(m_device->QueryInterface(__uuidof(IDXGIDevice),(void **)&device)<0)
            return;

        IDXGIAdapter *adaptor;
        if(device->GetAdapter(&adaptor)<0)
            return;

        IDXGIFactory2 *factory;
        if(adaptor->GetParent(__uuidof(IDXGIFactory2),(void **)&factory)<0)
            return;

        Windows::UI::Core::CoreWindow^ window = m_window.Get();
        if(factory->CreateSwapChainForCoreWindow(m_device,reinterpret_cast<IUnknown*>(window),&swapChainDesc,nullptr,&m_swap_chain)<0)
            return;

        if(device->SetMaximumFrameLatency(1)<0)
            return;

        ID3D11Texture2D *back_buffer;
        if(m_swap_chain->GetBuffer(0,__uuidof(ID3D11Texture2D),(void **)&back_buffer)<0)
            return;

        if(m_device->CreateRenderTargetView(back_buffer,nullptr,&m_renderTargetView)<0)
            return;

        CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT,static_cast<UINT>(m_renderTargetSize.Width),
                                               static_cast<UINT>(m_renderTargetSize.Height),1,1,D3D11_BIND_DEPTH_STENCIL);
        ID3D11Texture2D *depthStencil;
        if(m_device->CreateTexture2D(&depthStencilDesc,nullptr,&depthStencil)<0)
            return;

        CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
        if(m_device->CreateDepthStencilView(depthStencil,&depthStencilViewDesc,&m_depthStencilView)<0)
            return;

        nya_render::set_default_target(m_renderTargetView,m_depthStencilView);
        nya_render::set_viewport(0,0,(int)m_renderTargetSize.Width,(int)m_renderTargetSize.Height);
    }

private:
    PhoneDirect3DApp(nya_system::app &app): m_app(app),m_windowClosed(false),m_windowVisible(false),m_time(0),m_height(0) {}

private:
    nya_system::app &m_app;

private:
    bool m_windowClosed;
    bool m_windowVisible;
    unsigned long m_time;
    int m_height;

private:
    ID3D11Device *m_device;
    ID3D11DeviceContext *m_context;
    D3D_FEATURE_LEVEL m_featureLevel;

private:
    Windows::Foundation::Size m_renderTargetSize;
    Windows::Foundation::Rect m_windowBounds;
    Platform::Agile<Windows::UI::Core::CoreWindow> m_window;

private:
    ID3D11RenderTargetView *m_renderTargetView;
    ID3D11DepthStencilView *m_depthStencilView;
    IDXGISwapChain1 *m_swap_chain;
};

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
    friend class shared_app;
public:
    virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView()
    {
        return ref new PhoneDirect3DApp(m_app);
    }

private:
    Direct3DApplicationSource(nya_system::app &app): m_app(app) {}

private:
    nya_system::app &m_app;
};

class shared_app
{
public:
    void start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
    {
        auto direct3DApplicationSource = ref new Direct3DApplicationSource(app);
        CoreApplication::Run(direct3DApplicationSource);
    }

    void start_fullscreen(unsigned int w,unsigned int h,int aa,nya_system::app &app)
    {
        start_windowed(0,0,w,h,aa,app);
    }

    void finish(nya_system::app &app)
    {
    }

    void set_title(const char *title)
    {
    }
	
	void set_virtual_keyboard(int type) 
	{
	}

public:
    static shared_app &get_app()
    {
        static shared_app app;
        return app;
    }
};

}

  #else

    #include <windowsx.h>

  #ifndef DIRECTX11
    #include <gl/gl.h>
    #include <gl/wglext.h>
    #include <gl/glext.h>
    #include "render/render_opengl.h"
  #endif

namespace
{

class shared_app
{
public:
    void start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
    {
        m_instance=GetModuleHandle(NULL);
        if(!m_instance)
            return;

        WNDCLASS wc;
        wc.cbClsExtra=0;
        wc.cbWndExtra=0;
        wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.hCursor=LoadCursor(NULL,IDC_ARROW);
        wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);
        wc.hInstance=m_instance;
        wc.lpfnWndProc=wnd_proc;
        wc.lpszClassName=TEXT("nya_engine");
        wc.lpszMenuName=0;
        wc.style=CS_HREDRAW|CS_VREDRAW|CS_OWNDC;

        if(!RegisterClass(&wc))
            return;

        RECT rect = {x,y,int(x+w),int(y+h)};
        AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,false);

        m_hwnd = CreateWindowA("nya_engine",
                          m_title.c_str(),
                          WS_OVERLAPPEDWINDOW,
                          rect.left,rect.top,
                          rect.right-rect.left,rect.bottom-rect.top,
                          NULL,NULL,m_instance, NULL);

        if(!m_hwnd)
            return;

        ShowWindow(m_hwnd,SW_SHOW);

  #ifdef DIRECTX11
        UINT create_device_flags=0;
    #ifdef _DEBUG
        //create_device_flags|=D3D11_CREATE_DEVICE_DEBUG;
    #endif

        D3D_DRIVER_TYPE driver_types[]=
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };

        D3D_FEATURE_LEVEL feature_levels[]=
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };

        D3D_FEATURE_LEVEL feature_level=D3D_FEATURE_LEVEL_11_0;

        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd,sizeof(sd));
        sd.BufferCount=1;
        sd.BufferDesc.Width=w;
        sd.BufferDesc.Height=h;
        sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator=60;
        sd.BufferDesc.RefreshRate.Denominator=1;
        sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow=m_hwnd;
        sd.SampleDesc.Count=1;
        sd.SampleDesc.Quality=0;
        sd.Windowed=TRUE;

        HRESULT hr = S_OK;

        for(int i=0;i<ARRAYSIZE(driver_types);++i)
        {
            D3D_DRIVER_TYPE driver_type=driver_types[i];
            hr=D3D11CreateDeviceAndSwapChain(0,driver_type,0,create_device_flags,feature_levels,
            ARRAYSIZE(feature_levels),D3D11_SDK_VERSION,&sd,&m_swap_chain,&m_device,&feature_level,&m_context);
            if(SUCCEEDED(hr))
                break;
        }
        if(FAILED(hr))
            return;

        nya_render::set_context(m_context);
        nya_render::set_device(m_device);
        nya_render::cull_face::disable();
        recreate_targets(w,h);
  #else
        m_hdc=GetDC(m_hwnd);

        PIXELFORMATDESCRIPTOR pfd={0};
        pfd.nSize=sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion=1;
        pfd.dwFlags=PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER|PFD_DRAW_TO_WINDOW;
        pfd.iPixelType=PFD_TYPE_RGBA;
        pfd.cColorBits=24;
        pfd.cAlphaBits=8;
        pfd.cDepthBits=24;

        int pf=ChoosePixelFormat(m_hdc,&pfd);
        if(!pf)
            return;

        if(!SetPixelFormat(m_hdc,pf,&pfd))
            return;

        m_hglrc=wglCreateContext(m_hdc);
        if(!m_hglrc)
            return;

        wglMakeCurrent(m_hdc,m_hglrc);

        if(antialiasing>0)
        {
            if(!nya_render::render_opengl::has_extension("GL_ARB_multisample"))
            {
                //antialiasing=0;
                nya_system::log()<<"GL_ARB_multisample not found\n";
            }
        }

        PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB=0;
        if(antialiasing>0)
        {
            wglChoosePixelFormatARB =
            (PFNWGLCHOOSEPIXELFORMATARBPROC)nya_render::render_opengl::get_extension("wglChoosePixelFormatARB");
            if(!wglChoosePixelFormatARB)
            {
                antialiasing=0;
                nya_system::log()<<"wglChoosePixelFormatARB not found\n";
            }
        }

        UINT num_aa_formats=0;
        int aa_pf=0;

        if(antialiasing>0)
        {
            int iAttributes[] = { WGL_DRAW_TO_WINDOW_ARB,GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB,GL_TRUE,
            WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
            WGL_COLOR_BITS_ARB,24,
            WGL_ALPHA_BITS_ARB,8,
            WGL_DEPTH_BITS_ARB,24,
            WGL_STENCIL_BITS_ARB,0,
            WGL_DOUBLE_BUFFER_ARB,GL_TRUE,
            WGL_SAMPLE_BUFFERS_ARB,GL_TRUE,
            WGL_SAMPLES_ARB,antialiasing,
            0,0};

            nya_system::log()<<"antialiasing init\n";

            if(!wglChoosePixelFormatARB(m_hdc,iAttributes,0,1,&aa_pf,&num_aa_formats))
            {
                nya_system::log()<<"wglChoosePixelFormatARB failed\n";
                antialiasing=0;
            }
        }

        if(antialiasing>0)
        {
            wglMakeCurrent (m_hdc, 0);
            wglDeleteContext(m_hglrc);
            ReleaseDC(m_hwnd,m_hdc);
            DestroyWindow (m_hwnd);

            m_hwnd = CreateWindowA("nya_engine",
                          m_title.c_str(),
                          WS_OVERLAPPEDWINDOW,
                          rect.left,rect.top,
                          rect.right-rect.left,rect.bottom-rect.top,
                          NULL,NULL,m_instance, NULL);

            ShowWindow(m_hwnd,SW_SHOW);
            m_hdc=GetDC(m_hwnd);

            if(num_aa_formats>=1 && SetPixelFormat(m_hdc,aa_pf,&pfd))
            {
                nya_system::log()<<"antialiasiing is set\n";
            }
            else
            {
                antialiasing=0;
                nya_system::log()<<"unable to set antialiasiing "<<aa_pf<<" "<<num_aa_formats<<"\n";

                int pf=ChoosePixelFormat(m_hdc,&pfd);
                if(!pf)
                    return;

                if(!SetPixelFormat(m_hdc,pf,&pfd))
                    return;
            }

            m_hglrc=wglCreateContext(m_hdc);
            if(!m_hglrc)
                return;

            wglMakeCurrent(m_hdc,m_hglrc);
        }

        if(antialiasing>1)
            glEnable(GL_MULTISAMPLE_ARB);
  #endif
        SetWindowTextA(m_hwnd,m_title.c_str());

        SetWindowLongPtr(m_hwnd,GWLP_USERDATA,(LONG_PTR)&app);

        nya_render::set_viewport(0,0,w,h);
        app.on_resize(w,h);
        m_time=nya_system::get_time();

        if(app.on_splash())
        {
#ifdef DIRECTX11
            m_swap_chain->Present(0,0);
#else
            SwapBuffers(m_hdc);
#endif        
        }

        app.on_init();

        m_time=nya_system::get_time();

        MSG msg;
        while(m_hwnd)
        {
            if(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
            {
                if(msg.message==WM_QUIT)
                    break;

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                unsigned long time=nya_system::get_time();
                unsigned int dt=(unsigned)(time-m_time);
                m_time=time;

                app.on_frame(dt);

  #ifdef DIRECTX11
                m_swap_chain->Present(0,0);
  #else
                SwapBuffers(m_hdc);
  #endif
            }
        }

        finish(app);
    }

    void start_fullscreen(unsigned int w,unsigned int h,int antialiasing,nya_system::app &app)
    {
        //ToDo

        start_windowed(0,0,w,h,0,app);
    }

    void finish(nya_system::app &app)
    {
        if(!m_hwnd)
            return;

        app.on_free();

  #ifdef DIRECTX11
        if(m_context)
            m_context->ClearState();

        if(m_color_target)
        {
            m_color_target->Release();
            m_color_target=0;
        }

        if( m_depth_target )
        {
            m_depth_target->Release();
            m_depth_target=0;
        }

        if(m_swap_chain)
        {
            m_swap_chain->Release();
            m_swap_chain=0;
        }

        if(m_context)
        {
            m_context->Release();
            m_context=0;
        }

        if(m_device)
        {
            m_device->Release();
            m_device=0;
        }
  #else
        wglMakeCurrent (m_hdc, 0);
        wglDeleteContext(m_hglrc);
        ReleaseDC(m_hwnd,m_hdc);
        DestroyWindow (m_hwnd);
  #endif
        m_hwnd=0;
    }

  #ifdef DIRECTX11
private:
    bool recreate_targets(int w,int h)
    {
        HRESULT hr=S_OK;

        nya_render::set_default_target(0,0);

        if(m_color_target)
        {
            m_color_target->Release();
            m_color_target=0;
        }

        if(m_depth_target)
        {
            m_depth_target->Release();
            m_depth_target=0;
        }

        hr=m_swap_chain->ResizeBuffers(0,0,0,DXGI_FORMAT_UNKNOWN,0);
        if(FAILED(hr))
            return false;

        ID3D11Texture2D* pBackBuffer=0;
        hr=m_swap_chain->GetBuffer(0,__uuidof(ID3D11Texture2D ),(LPVOID*)&pBackBuffer);
        if(FAILED(hr))
            return false;

        hr=m_device->CreateRenderTargetView(pBackBuffer,0,&m_color_target);
        pBackBuffer->Release();
        if(FAILED(hr))
            return false;

        CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT,w,h,1,1,D3D11_BIND_DEPTH_STENCIL);

        ID3D11Texture2D *depthStencil;
        hr=m_device->CreateTexture2D(&depthStencilDesc,nullptr,&depthStencil);
        if(FAILED(hr))
            return false;

        CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
        m_device->CreateDepthStencilView(depthStencil,&depthStencilViewDesc,&m_depth_target);
        depthStencil->Release();
        
        nya_render::set_default_target(m_color_target,m_depth_target);
        return true;
    }
  #endif

private:
	static unsigned int get_x11_key(unsigned int key)
	{
        if(key>='A' && key<='Z')
            return nya_system::key_a+key-'A';

        if(key>='0' && key<='9')
            return nya_system::key_0+key-'0';

        if(key>=VK_F1 && key<=VK_F12)
            return nya_system::key_f1+key-VK_F1;

		switch(key)
		{
            case VK_SHIFT: return nya_system::key_shift;
            case VK_CONTROL: return nya_system::key_control;
            case VK_MENU: return nya_system::key_alt;

            case VK_CAPITAL: return nya_system::key_capital;
            case VK_ESCAPE: return nya_system::key_escape;
            case VK_SPACE: return nya_system::key_space;
            case VK_RETURN: return nya_system::key_return;
            case VK_TAB: return nya_system::key_tab;

            case VK_PRIOR: return nya_system::key_page_up;
            case VK_NEXT: return nya_system::key_page_down;
            case VK_END: return nya_system::key_end;
            case VK_HOME: return nya_system::key_home;
            case VK_INSERT: return nya_system::key_insert;
            case VK_DELETE: return nya_system::key_delete;
            case VK_BACK: return nya_system::key_backspace;

            case VK_UP: return nya_system::key_up;
            case VK_DOWN: return nya_system::key_down;
            case VK_LEFT: return nya_system::key_left;
            case VK_RIGHT: return nya_system::key_right;

            case VK_OEM_4: return nya_system::key_bracket_left;
            case VK_OEM_6: return nya_system::key_bracket_right;
            case VK_OEM_COMMA: return nya_system::key_comma;
            case VK_OEM_PERIOD: return nya_system::key_period;
		}

		return 0;
	}

    static LRESULT CALLBACK wnd_proc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam)
    {
        nya_system::app *app=(nya_system::app*)GetWindowLongPtr(hwnd,GWLP_USERDATA);
        if(!app)
            return DefWindowProc(hwnd,message,wparam,lparam );

        switch(message)
        {
            case WM_SIZE:
            {
                RECT rc;
                GetClientRect(hwnd,&rc);

                const int w=rc.right-rc.left;
                const int h=rc.bottom-rc.top;

  #ifdef DIRECTX11
                get_app().recreate_targets(w,h);
  #endif
                nya_render::set_viewport(0,0,w,h);
                app->on_resize(w,h);
            }
            break;

            case WM_CLOSE: get_app().finish(*app); break;

            case WM_MOUSEWHEEL:
            {
                const int x=GET_X_LPARAM(wparam);
                const int y=GET_Y_LPARAM(wparam);

                app->on_mouse_scroll(x/60,y/60);
            }
            break;

            case WM_MOUSEMOVE:
            {
                const int x=LOWORD(lparam);
                const int y=HIWORD(lparam);

                RECT rc;
                GetClientRect(hwnd,&rc);

                app->on_mouse_move(x,rc.bottom+rc.top-y);
            }
            break;

            case WM_LBUTTONDOWN: app->on_mouse_button(nya_system::mouse_left,true); break;
            case WM_LBUTTONUP: app->on_mouse_button(nya_system::mouse_left,false); break;
            case WM_MBUTTONDOWN: app->on_mouse_button(nya_system::mouse_middle,true); break;
            case WM_MBUTTONUP: app->on_mouse_button(nya_system::mouse_middle,false); break;
            case WM_RBUTTONDOWN: app->on_mouse_button(nya_system::mouse_right,true); break;
            case WM_RBUTTONUP: app->on_mouse_button(nya_system::mouse_right,false); break;

            case WM_KEYDOWN:
            {
                const unsigned int key=LOWORD(wparam);
				const unsigned int x11key=get_x11_key(key);
				if(x11key)
					app->on_keyboard(x11key,true);
            }
            break;

            case WM_KEYUP:
            {
                const unsigned int key=LOWORD(wparam);
				const unsigned int x11key=get_x11_key(key);
				if(x11key)
					app->on_keyboard(x11key,false);
            }
            break;

            case WM_CHAR:
            {
                const unsigned int key=wparam;
                const bool pressed=((lparam & (1<<31))==0);
                const bool autorepeat=((lparam & 0xff)!=0);
				app->on_charcode(key,pressed,autorepeat);
            }
            break;

            case WM_SYSCOMMAND:
            {
                if (wparam == SC_MINIMIZE && !m_suspended)
                {
                    m_suspended = true;
                    app->on_suspend();
                }
                else if (wparam == SC_RESTORE && m_suspended)
                {
                    m_suspended = false;
                    app->on_restore();
                }
            }
            break;
        };

        return DefWindowProc(hwnd,message,wparam,lparam );
    }

public:
    void set_title(const char *title)
    {
        if(!title)
        {
            m_title.clear();
            return;
        }

        m_title.assign(title);

        if(m_hwnd)
            SetWindowTextA(m_hwnd,title);
    }
	void set_virtual_keyboard(int type) {}

public:
    static shared_app &get_app()
    {
        static shared_app app;
        return app;
    }

public:
    shared_app():
  #ifdef DIRECTX11
        m_device(0),
        m_context(0),
        m_swap_chain(0),
        m_color_target(0),
        m_depth_target(0),
  #else
        m_hdc(0),
  #endif
        m_title("Nya engine"),m_time(0) {}

private:
    HINSTANCE m_instance;
    HWND m_hwnd;
  #ifdef DIRECTX11
    ID3D11Device* m_device;
    ID3D11DeviceContext* m_context;
    IDXGISwapChain* m_swap_chain;
    ID3D11RenderTargetView* m_color_target;
    ID3D11DepthStencilView* m_depth_target;
  #else
    HDC m_hdc;
    HGLRC m_hglrc;
  #endif

    std::string m_title;
    unsigned long m_time;

    static bool m_suspended;
};

bool shared_app::m_suspended = false;

}
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

void app::start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing)
{
    shared_app::get_app().start_windowed(x,y,w,h,antialiasing,*this);
}

void app::start_fullscreen(unsigned int w,unsigned int h,int aa)
{
    shared_app::get_app().start_fullscreen(w,h,aa,*this);
}

void app::set_title(const char *title)
{
    shared_app::get_app().set_title(title);
}

void app::set_virtual_keyboard(virtual_keyboard_type type)
{
    shared_app::get_app().set_virtual_keyboard(type);
}

void app::set_mouse_pos(int x,int y)
{
}

void app::finish()
{
    shared_app::get_app().finish(*this);
}

}

#endif
