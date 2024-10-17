// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// Drop support for METRO, FLUENT style.
// Update the code to be compatible with the latest version of the engine.
// Optimisation and code cleaning for a better performance.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#include "app.h"
#include "system.h"
#include "render/render.h"

#include <string>

//MARK: Windows
#ifdef _WIN32

#include <windows.h>

// This Macro (_MSC_VER) is defined by the Microsoft C/C++ compiler.
#if defined(_MSC_VER) && _MSC_VER >= 1900 // VS2015
// This Macro (_WIN32_WINNT) is defined by the Windows SDK.
#if _WIN32_WINNT >= _WIN32_WINNT_WIN10

#include "winapifamily.h"

#endif
#endif

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
    void start_windowed(int x,
                        int y,
                        unsigned int w,
                        unsigned int h,
                        int antialiasing,
                        rox_system::app& app)
    {
        m_instance = GetModuleHandle(NULL);
        if (!m_instance)
            return;
        
        WNDCLASS wc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.hCursor = LoadCursor(NULL,
                                IDC_ARROW);
        wc.hIcon = LoadIcon(NULL,
                            IDI_APPLICATION);
        wc.hInstance = m_instance;
        wc.lpfnWndProc = wnd_proc;
        wc.lpszClassName = TEXT("nya_engine");
        wc.lpszMenuName = 0;
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        
        if (!RegisterClass(&wc))
            return;
        
        RECT rect = {
            x,
            y,
            int(x + w),
            int(y + h)
        };
        AdjustWindowRect(&rect,
                         WS_OVERLAPPEDWINDOW,
                         false);
        
        m_hwnd = CreateWindowA("nya_engine",
                               m_title
                               .c_str(),
                               WS_OVERLAPPEDWINDOW,
                               rect.left,
                               rect.top,
                               rect.right - rect.left,
                               rect.bottom - rect.top,
                               NULL,
                               NULL,
                               m_instance,
                               NULL);
        
        if (!m_hwnd)
            return;
        
        ShowWindow(m_hwnd,
                   SW_SHOW);
        
#ifdef DIRECTX11
        UINT create_device_flags = 0;
#ifdef _DEBUG
        //create_device_flags|=D3D11_CREATE_DEVICE_DEBUG;
#endif
        
        D3D_DRIVER_TYPE driver_types[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
        
        D3D_FEATURE_LEVEL feature_levels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
        };
        
        D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
        
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory(&sd,
                   sizeof(sd));
        sd.BufferCount = 1;
        sd.BufferDesc.Width = w;
        sd.BufferDesc.Height = h;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = m_hwnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;
        
        HRESULT hr = S_OK;
        
        for (int i = 0; i < ARRAYSIZE(driver_types); ++i)
        {
            D3D_DRIVER_TYPE driver_type = driver_types[i];
            hr = D3D11CreateDeviceAndSwapChain(0,
                                               driver_type,
                                               0,
                                               create_device_flags,
                                               feature_levels,
                                               ARRAYSIZE(feature_levels),
                                               D3D11_SDK_VERSION,
                                               &sd,
                                               &m_swap_chain,
                                               &m_device,
                                               &feature_level,
                                               &m_context);
            if (SUCCEEDED(hr))
                break;
        }
        if (FAILED(hr))
            return;
        
        nya_render::set_context(m_context);
        nya_render::set_device(m_device);
        nya_render::cull_face::disable();
        recreate_targets(w,
                         h);
#else
        m_hdc = GetDC(m_hwnd);
        
        PIXELFORMATDESCRIPTOR pfd = {
            0
        };
        pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
        pfd.nVersion = 1;
        pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 24;
        pfd.cAlphaBits = 8;
        pfd.cDepthBits = 24;
        
        int pf = ChoosePixelFormat(m_hdc,
                                   &pfd);
        if (!pf)
            return;
        
        if (!SetPixelFormat(m_hdc,
                            pf,
                            &pfd))
            return;
        
        m_hglrc = wglCreateContext(m_hdc);
        if (!m_hglrc)
            return;
        
        wglMakeCurrent(m_hdc,
                       m_hglrc);
        
        if (antialiasing > 0)
        {
            if (!nya_render::render_opengl::has_extension("GL_ARB_multisample"))
            {
                //antialiasing=0;
                rox_system::log() << "GL_ARB_multisample not found\n";
            }
        }
        
        PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = 0;
        if (antialiasing > 0)
        {
            wglChoosePixelFormatARB =
            (PFNWGLCHOOSEPIXELFORMATARBPROC)nya_render::render_opengl::get_extension("wglChoosePixelFormatARB");
            if (!wglChoosePixelFormatARB)
            {
                antialiasing = 0;
                rox_system::log() << "wglChoosePixelFormatARB not found\n";
            }
        }
        
        UINT num_aa_formats = 0;
        int aa_pf = 0;
        
        if (antialiasing > 0)
        {
            int iAttributes[] = {
                WGL_DRAW_TO_WINDOW_ARB,
                GL_TRUE,
                WGL_SUPPORT_OPENGL_ARB,
                GL_TRUE,
                WGL_ACCELERATION_ARB,
                WGL_FULL_ACCELERATION_ARB,
                WGL_COLOR_BITS_ARB,
                24,
                WGL_ALPHA_BITS_ARB,
                8,
                WGL_DEPTH_BITS_ARB,
                24,
                WGL_STENCIL_BITS_ARB,
                0,
                WGL_DOUBLE_BUFFER_ARB,
                GL_TRUE,
                WGL_SAMPLE_BUFFERS_ARB,
                GL_TRUE,
                WGL_SAMPLES_ARB,
                antialiasing,
                0,
                0
            };
            
            rox_system::log() << "antialiasing init\n";
            
            if (!wglChoosePixelFormatARB(m_hdc,
                                         iAttributes,
                                         0,
                                         1,
                                         &aa_pf,
                                         &num_aa_formats))
            {
                rox_system::log() << "wglChoosePixelFormatARB failed\n";
                antialiasing = 0;
            }
        }
        
        if (antialiasing > 0)
        {
            wglMakeCurrent(m_hdc,
                           0);
            wglDeleteContext(m_hglrc);
            ReleaseDC(m_hwnd,
                      m_hdc);
            DestroyWindow(m_hwnd);
            
            m_hwnd = CreateWindowA("nya_engine",
                                   m_title
                                   .c_str(),
                                   WS_OVERLAPPEDWINDOW,
                                   rect.left,
                                   rect.top,
                                   rect.right - rect.left,
                                   rect.bottom - rect.top,
                                   NULL,
                                   NULL,
                                   m_instance,
                                   NULL);
            
            ShowWindow(m_hwnd,
                       SW_SHOW);
            m_hdc = GetDC(m_hwnd);
            
            if (num_aa_formats >= 1 && SetPixelFormat(m_hdc,
                                                      aa_pf,
                                                      &pfd))
            {
                rox_system::log() << "antialiasiing is set\n";
            }
            else
            {
                antialiasing = 0;
                rox_system::log() << "unable to set antialiasiing " << aa_pf << " " << num_aa_formats << "\n";
                
                int pf = ChoosePixelFormat(m_hdc,
                                           &pfd);
                if (!pf)
                    return;
                
                if (!SetPixelFormat(m_hdc,
                                    pf,
                                    &pfd))
                    return;
            }
            
            m_hglrc = wglCreateContext(m_hdc);
            if (!m_hglrc)
                return;
            
            wglMakeCurrent(m_hdc,
                           m_hglrc);
        }
        
        if (antialiasing > 1)
            glEnable(GL_MULTISAMPLE_ARB);
#endif
        SetWindowTextA(m_hwnd,
                       m_title
                       .c_str());
        
        SetWindowLongPtr(m_hwnd,
                         GWLP_USERDATA,
                         (LONG_PTR)&app);
        
        nya_render::set_viewport(0,
                                 0,
                                 w,
                                 h);
        app
            .on_resize(w,
                       h);
        m_time = rox_system::get_time();
        
        if (app.on_splash())
        {
#ifdef DIRECTX11
            m_swap_chain->Present(0,
                                  0);
#else
            SwapBuffers(m_hdc);
#endif        
        }
        
        app
            .on_init();
        
        m_time = rox_system::get_time();
        
        MSG msg;
        while (m_hwnd)
        {
            if (PeekMessage(&msg,
                            NULL,
                            0,
                            0,
                            PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                    break;
                
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                unsigned long time = rox_system::get_time();
                unsigned int dt = (unsigned)(time - m_time);
                m_time = time;
                
                app
                    .on_frame(dt);
                
#ifdef DIRECTX11
                m_swap_chain->Present(0,
                                      0);
#else
                SwapBuffers(m_hdc);
#endif
            }
        }
        
        finish(app);
    }
    
    void start_fullscreen(unsigned int w,
                          unsigned int h,
                          int antialiasing,
                          rox_system::app& app)
    {
        //ToDo
        
        start_windowed(0,
                       0,
                       w,
                       h,
                       0,
                       app);
    }
    
    void finish(rox_system::app& app)
    {
        if (!m_hwnd)
            return;
        
        app
            .on_free();
        
#ifdef DIRECTX11
        if (m_context)
            m_context->ClearState();
        
        if (m_color_target)
        {
            m_color_target->Release();
            m_color_target = 0;
        }
        
        if (m_depth_target)
        {
            m_depth_target->Release();
            m_depth_target = 0;
        }
        
        if (m_swap_chain)
        {
            m_swap_chain->Release();
            m_swap_chain = 0;
        }
        
        if (m_context)
        {
            m_context->Release();
            m_context = 0;
        }
        
        if (m_device)
        {
            m_device->Release();
            m_device = 0;
        }
#else
        wglMakeCurrent(m_hdc,
                       0);
        wglDeleteContext(m_hglrc);
        ReleaseDC(m_hwnd,
                  m_hdc);
        DestroyWindow(m_hwnd);
#endif
        m_hwnd = 0;
    }
    
#ifdef DIRECTX11
private:
    bool recreate_targets(int w,
                          int h)
    {
        HRESULT hr = S_OK;
        
        nya_render::set_default_target(0,
                                       0);
        
        if (m_color_target)
        {
            m_color_target->Release();
            m_color_target = 0;
        }
        
        if (m_depth_target)
        {
            m_depth_target->Release();
            m_depth_target = 0;
        }
        
        hr = m_swap_chain->ResizeBuffers(0,
                                         0,
                                         0,
                                         DXGI_FORMAT_UNKNOWN,
                                         0);
        if (FAILED(hr))
            return false;
        
        ID3D11Texture2D* pBackBuffer = 0;
        hr = m_swap_chain->GetBuffer(0,
                                     __uuidof(ID3D11Texture2D),
                                     (LPVOID*)&pBackBuffer);
        if (FAILED(hr))
            return false;
        
        hr = m_device->CreateRenderTargetView(pBackBuffer,
                                              0,
                                              &m_color_target);
        pBackBuffer->Release();
        if (FAILED(hr))
            return false;
        
        CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT,
                                               w,
                                               h,
                                               1,
                                               1,
                                               D3D11_BIND_DEPTH_STENCIL);
        
        ID3D11Texture2D* depthStencil;
        hr = m_device->CreateTexture2D(&depthStencilDesc,
                                       nullptr,
                                       &depthStencil);
        if (FAILED(hr))
            return false;
        
        CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
        m_device->CreateDepthStencilView(depthStencil,
                                         &depthStencilViewDesc,
                                         &m_depth_target);
        depthStencil->Release();
        
        nya_render::set_default_target(m_color_target,
                                       m_depth_target);
        return true;
    }
#endif
    
private:
    static unsigned int get_x11_key(unsigned int key)
    {
        if (key >= 'A' && key <= 'Z')
            return rox_system::key_a + key - 'A';
        
        if (key >= '0' && key <= '9')
            return rox_system::key_0 + key - '0';
        
        if (key >= VK_F1 && key <= VK_F12)
            return rox_system::key_f1 + key - VK_F1;
        
        switch (key)
        {
            case VK_SHIFT: return rox_system::key_shift;
            case VK_CONTROL: return rox_system::key_control;
            case VK_MENU: return rox_system::key_alt;
                
            case VK_CAPITAL: return rox_system::key_capital;
            case VK_ESCAPE: return rox_system::key_escape;
            case VK_SPACE: return rox_system::key_space;
            case VK_RETURN: return rox_system::key_return;
            case VK_TAB: return rox_system::key_tab;
                
            case VK_PRIOR: return rox_system::key_page_up;
            case VK_NEXT: return rox_system::key_page_down;
            case VK_END: return rox_system::key_end;
            case VK_HOME: return rox_system::key_home;
            case VK_INSERT: return rox_system::key_insert;
            case VK_DELETE: return rox_system::key_delete;
            case VK_BACK: return rox_system::key_backspace;
                
            case VK_UP: return rox_system::key_up;
            case VK_DOWN: return rox_system::key_down;
            case VK_LEFT: return rox_system::key_left;
            case VK_RIGHT: return rox_system::key_right;
                
            case VK_OEM_4: return rox_system::key_bracket_left;
            case VK_OEM_6: return rox_system::key_bracket_right;
            case VK_OEM_COMMA: return rox_system::key_comma;
            case VK_OEM_PERIOD: return rox_system::key_period;
        }
        
        return 0;
    }
    
    static LRESULT CALLBACK wnd_proc(HWND hwnd,
                                     UINT message,
                                     WPARAM wparam,
                                     LPARAM lparam)
    {
        rox_system::app* app = (rox_system::app*)GetWindowLongPtr(hwnd,
                                                                  GWLP_USERDATA);
        if (!app)
            return DefWindowProc(hwnd,
                                 message,
                                 wparam,
                                 lparam);
        
        switch (message)
        {
            case WM_SIZE:
            {
                RECT rc;
                GetClientRect(hwnd,
                              &rc);
                
                const int w = rc.right - rc.left;
                const int h = rc.bottom - rc.top;
                
#ifdef DIRECTX11
                get_app()
                    .recreate_targets(w,
                                      h);
#endif
                nya_render::set_viewport(0,
                                         0,
                                         w,
                                         h);
                app->on_resize(w,
                               h);
            }
                break;
                
            case WM_CLOSE: get_app()
                    .finish(*app); break;
                
            case WM_MOUSEWHEEL:
            {
                const int x = GET_X_LPARAM(wparam);
                const int y = GET_Y_LPARAM(wparam);
                
                app->on_mouse_scroll(x / 60,
                                     y / 60);
            }
                break;
                
            case WM_MOUSEMOVE:
            {
                const int x = LOWORD(lparam);
                const int y = HIWORD(lparam);
                
                RECT rc;
                GetClientRect(hwnd,
                              &rc);
                
                app->on_mouse_move(x,
                                   rc.bottom + rc.top - y);
            }
                break;
                
            case WM_LBUTTONDOWN: app->on_mouse_button(rox_system::mouse_left,
                                                      true); break;
            case WM_LBUTTONUP: app->on_mouse_button(rox_system::mouse_left,
                                                    false); break;
            case WM_MBUTTONDOWN: app->on_mouse_button(rox_system::mouse_middle,
                                                      true); break;
            case WM_MBUTTONUP: app->on_mouse_button(rox_system::mouse_middle,
                                                    false); break;
            case WM_RBUTTONDOWN: app->on_mouse_button(rox_system::mouse_right,
                                                      true); break;
            case WM_RBUTTONUP: app->on_mouse_button(rox_system::mouse_right,
                                                    false); break;
                
            case WM_KEYDOWN:
            {
                const unsigned int key = LOWORD(wparam);
                const unsigned int x11key = get_x11_key(key);
                if (x11key)
                    app->on_keyboard(x11key,
                                     true);
            }
                break;
                
            case WM_KEYUP:
            {
                const unsigned int key = LOWORD(wparam);
                const unsigned int x11key = get_x11_key(key);
                if (x11key)
                    app->on_keyboard(x11key,
                                     false);
            }
                break;
                
            case WM_CHAR:
            {
                const unsigned int key = wparam;
                const bool pressed = ((lparam & (1 << 31)) == 0);
                const bool autorepeat = ((lparam & 0xff) != 0);
                app->on_charcode(key,
                                 pressed,
                                 autorepeat);
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
        
        return DefWindowProc(hwnd,
                             message,
                             wparam,
                             lparam);
    }
    
public:
    void set_title(const char* title)
    {
        if (!title)
        {
            m_title
                .clear();
            return;
        }
        
        m_title
            .assign(title);
        
        if (m_hwnd)
            SetWindowTextA(m_hwnd,
                           title);
    }

	std::string get_title()
	{
		return m_title;
	}

    void set_virtual_keyboard(int type) {
    }
    
public:
    static shared_app& get_app()
    {
        static shared_app app;
        return app;
    }
    
public:
    shared_app() :
#ifdef DIRECTX11
    m_device(0),
    m_context(0),
    m_swap_chain(0),
    m_color_target(0),
    m_depth_target(0),
#else
    m_hdc(0),
#endif
    m_title("Nya engine"), m_time(0) {
    }
    
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

//MARK: Apple
#elif defined __APPLE__ //implemented in app.mm

#endif


#ifndef __APPLE__ //implemented in app.mm

namespace rox_system
{
void app::start_windowed(int x,
                         int y,
                         unsigned int w,
                         unsigned int h,
                         int antialiasing)
{
    shared_app::get_app()
        .start_windowed(x,
                        y,
                        w,
                        h,
                        antialiasing,
                        *this);
}

void app::start_fullscreen(unsigned int w,
                           unsigned int h,
                           int aa)
{
    shared_app::get_app()
        .start_fullscreen(w,
                          h,
                          aa,
                          *this);
}

void app::set_title(const char* title)
{
    shared_app::get_app()
        .set_title(title);
}

std::string app::get_title()
{
    return shared_app::get_app()
        .get_title();
}

void app::set_virtual_keyboard(virtual_keyboard_type type)
{
    shared_app::get_app()
        .set_virtual_keyboard(type);
}

void app::set_mouse_pos(int x,
                        int y)
{
}

void app::finish()
{
    shared_app::get_app()
        .finish(*this);
	}
}

#endif
