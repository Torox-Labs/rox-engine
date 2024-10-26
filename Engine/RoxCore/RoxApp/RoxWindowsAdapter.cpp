// Updated by the Rox-engine
// Copyright © 2024 Torox Project
//
// This file is part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License: for non-commercial and commercial use under specific conditions.
// 2. Commercial License: for use on proprietary platforms.
// 
// For full licensing terms, please refer to the LICENSE file in the root directory of this project.


#include "RoxWindowsAdapter.h"
#include "RoxSystem/RoxSystem.h"
#include "RoxRender/RoxRender.h"

#ifdef _WIN32

#ifndef DIRECTX11
#include <gl/gl.h>
#include <gl/wglext.h>
#include <gl/glext.h>
#include "RoxRender/RoxRenderOpengl.h"
#endif

namespace RoxApp
{
	// Initialize static member variable
	bool RoxWindowsAdapter::m_suspended = false;

	RoxWindowsAdapter::RoxWindowsAdapter()
#ifdef DIRECTX11
		: m_device(nullptr), m_context(nullptr), m_swap_chain(nullptr), m_color_target(nullptr), m_depth_target(nullptr),
#else
		: m_hdc(nullptr),
#endif
		m_hwnd(nullptr), m_title("Rox engine"), m_time(0)
	{
	}

	RoxWindowsAdapter::~RoxWindowsAdapter()
	{
		// Cleanup resources if necessary
	}

	RoxWindowsAdapter& RoxWindowsAdapter::getInstance()
	{
		static RoxWindowsAdapter instance;
		return instance;
	}

	void RoxWindowsAdapter::startWindowed(int x, int y, unsigned int w, unsigned int h, int antialiasing, RoxApp& app)
	{
		m_instance = GetModuleHandle(nullptr);
		if (!m_instance)
			return;

		WNDCLASS wc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wc.hInstance = m_instance;
		wc.lpfnWndProc = wnd_proc;
		wc.lpszClassName = TEXT("rox_engine");
		wc.lpszMenuName = nullptr;
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;

		if (!RegisterClass(&wc))
			return;

		RECT rect = {
			x,
			y,
			static_cast<int>(x + w),
			static_cast<int>(y + h)
		};
		AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);

		m_hwnd = CreateWindowA("rox_engine",
			m_title.c_str(),
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

		ShowWindow(m_hwnd, SW_SHOW);


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

		RoxRender::set_context(m_context);
		RoxRender::set_device(m_device);
		RoxRender::cull_face::disable();
		recreate_targets(w,
			h);
#else

		// OpenGl initialization
		m_hdc = GetDC(m_hwnd); // Get the device context for the window
		PIXELFORMATDESCRIPTOR pfd = { 0 };

		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 24;
		pfd.cAlphaBits = 8;
		pfd.cDepthBits = 24;

		int pf = ChoosePixelFormat(m_hdc, &pfd);

		if (!pf)
			return;

		if (!SetPixelFormat(m_hdc, pf, &pfd))
			return;

		m_hglrc = wglCreateContext(m_hdc);
		if (!m_hglrc)
			return;

		wglMakeCurrent(m_hdc,
			m_hglrc);

		if (antialiasing > 0)
		{
			if (!RoxRender::RoxRenderOpengl::hasExtension("GL_ARB_multisample"))
			{
				//antialiasing=0;
				RoxSystem::log() << "GL_ARB_multisample not found\n";
			}
		}

		PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
		if (antialiasing > 0)
		{
			wglChoosePixelFormatARB =
				static_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(RoxRender::RoxRenderOpengl::getExtension(
					"wglChoosePixelFormatARB"));

			if (!wglChoosePixelFormatARB)
			{
				antialiasing = 0;
				RoxSystem::log() << "wglChoosePixelFormatARB not found\n";
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

			RoxSystem::log() << "antialiasing init\n";

			if (!wglChoosePixelFormatARB(m_hdc,
				iAttributes,
				nullptr,
				1,
				&aa_pf,
				&num_aa_formats))
			{
				RoxSystem::log() << "wglChoosePixelFormatARB failed\n";
				antialiasing = 0;
			}
		}

		if (antialiasing > 0)
		{
			wglMakeCurrent(m_hdc,
				nullptr);
			wglDeleteContext(m_hglrc);
			ReleaseDC(m_hwnd,
				m_hdc);
			DestroyWindow(m_hwnd);

			m_hwnd = CreateWindowA("rox_engine",
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
				RoxSystem::log() << "antialiasiing is set\n";
			}
			else
			{
				antialiasing = 0;
				RoxSystem::log() << "unable to set antialiasiing " << aa_pf << " " << num_aa_formats << "\n";

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

		// Set the viewport
		RoxRender::setViewport(0,
			0,
			w,
			h);
		app.onResize(w, h);

		m_time = RoxSystem::getTime();

		if (app.onSplash())
		{
#ifdef DIRECTX11
			m_swap_chain->Present(0,
				0);
#else
			SwapBuffers(m_hdc);
#endif
		}

		app.onInit();

		m_time = RoxSystem::getTime();

		MSG msg;

		// Main loop
		while (m_hwnd)
		{
			if (PeekMessage(&msg,
				nullptr,
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
				unsigned long time = RoxSystem::getTime();
				unsigned int dt = static_cast<unsigned>(time - m_time);
				m_time = time;

				app.onFrame(dt);

#ifdef DIRECTX11
				m_swap_chain->Present(0,
					0);
#else
				SwapBuffers(m_hdc); // Swap the buffers
#endif
			}
		}

		finish(app);
	}

	void RoxWindowsAdapter::startFullscreen(unsigned int w, unsigned int h, int antialiasing, RoxApp& app)
	{
		//ToDo

		startWindowed(0,
			0,
			w,
			h,
			0,
			app);
	}

	void RoxWindowsAdapter::setTitle(const char* title)
	{
		if (!title)
		{
			m_title.clear();
			return;
		}

		m_title.assign(title);

		if (m_hwnd)
			SetWindowTextA(m_hwnd, title);
	}

	std::string RoxWindowsAdapter::getTitle()
	{
		return m_title;
	}

	void RoxWindowsAdapter::setVirtualKeyboard(RoxInput::VIRTUAL_KEYBOARD_TYPE type)
	{
		//Todo : implement virtual keyboard

		// Implementation of setVirtualKeyboard function
		// ... (move your function body here)
	}

	void RoxWindowsAdapter::setMousePos(int x, int y)
	{
		if (m_hwnd)
			SetCursorPos(x, y);
	}

	void RoxWindowsAdapter::finish(RoxApp& app)
	{
		if (!m_hwnd)
			return;

		app.onFree();

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
			nullptr);
		wglDeleteContext(m_hglrc);
		ReleaseDC(m_hwnd,
			m_hdc);
		DestroyWindow(m_hwnd);

#endif

		m_hwnd = nullptr;
	}

	// Implement private helper functions
	unsigned int RoxWindowsAdapter::get_x11_key(unsigned int key)
	{
		if (key >= 'A' && key <= 'Z')
			return RoxInput::KEY_A + key - 'A';

		if (key >= '0' && key <= '9')
			return RoxInput::KEY_0 + key - '0';

		if (key >= VK_F1 && key <= VK_F12)
			return RoxInput::KEY_F1 + key - VK_F1;

		switch (key)
		{
		case VK_SHIFT: return RoxInput::KEY_SHIFT;
		case VK_CONTROL: return RoxInput::KEY_CONTROL;
		case VK_MENU: return RoxInput::KEY_ALT;

		case VK_CAPITAL: return RoxInput::KEY_CAPITAL;
		case VK_ESCAPE: return RoxInput::KEY_ESCAPE;
		case VK_SPACE: return RoxInput::KEY_SPACE;
		case VK_RETURN: return RoxInput::KEY_RETURN;
		case VK_TAB: return RoxInput::KEY_TAB;

		case VK_PRIOR: return RoxInput::KEY_PAGE_UP;
		case VK_NEXT: return RoxInput::KEY_PAGE_DOWN;
		case VK_END: return RoxInput::KEY_END;
		case VK_HOME: return RoxInput::KEY_HOME;
		case VK_INSERT: return RoxInput::KEY_INSERT;
		case VK_DELETE: return RoxInput::KEY_DELETE;
		case VK_BACK: return RoxInput::KEY_BACK;

		case VK_UP: return RoxInput::KEY_UP;
		case VK_DOWN: return RoxInput::KEY_DOWN;
		case VK_LEFT: return RoxInput::KEY_LEFT;
		case VK_RIGHT: return RoxInput::KEY_RIGHT;

		case VK_OEM_4: return RoxInput::KEY_BRACKET_LEFT;
		case VK_OEM_6: return RoxInput::KEY_BRACKET_RIGHT;
		case VK_OEM_COMMA: return RoxInput::KEY_COMMA;
		case VK_OEM_PERIOD: return RoxInput::KEY_PERIOD;
		}

		return 0;
	}

	LRESULT CALLBACK RoxWindowsAdapter::wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		auto app = (RoxApp*)GetWindowLongPtr(hwnd,
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
			getApp()
				.recreate_targets(w,
					h);
#endif

			RoxRender::setViewport(0,
				0,
				w,
				h);
			app->onResize(w,
				h);
		}
		break;

		case WM_CLOSE: getInstance()
			.finish(*app);
			break;

		case WM_MOUSEWHEEL:
		{
			const int x = GET_X_LPARAM(wparam);
			const int y = GET_Y_LPARAM(wparam);

			app->onMouseScroll(x / 60,
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

			app->onMouseMove(x,
				rc.bottom + rc.top - y);
		}
		break;

		case WM_LBUTTONDOWN: app->onMouseButton(RoxInput::MOUSE_LEFT,
			true);
			break;
		case WM_LBUTTONUP: app->onMouseButton(RoxInput::MOUSE_LEFT,
			false);
			break;
		case WM_MBUTTONDOWN: app->onMouseButton(RoxInput::MOUSE_MIDDLE,
			true);
			break;
		case WM_MBUTTONUP: app->onMouseButton(RoxInput::MOUSE_MIDDLE,
			false);
			break;
		case WM_RBUTTONDOWN: app->onMouseButton(RoxInput::MOUSE_RIGHT,
			true);
			break;
		case WM_RBUTTONUP: app->onMouseButton(RoxInput::MOUSE_RIGHT,
			false);
			break;

		case WM_KEYDOWN:
		{
			const unsigned int key = LOWORD(wparam);
			const unsigned int x11key = get_x11_key(key);
			if (x11key)
				app->onKeyboard(x11key,
					true);
		}
		break;

		case WM_KEYUP:
		{
			const unsigned int key = LOWORD(wparam);
			const unsigned int x11key = get_x11_key(key);
			if (x11key)
				app->onKeyboard(x11key,
					false);
		}
		break;

		case WM_CHAR:
		{
			const unsigned int key = wparam;
			const bool pressed = ((lparam & (1 << 31)) == 0);
			const bool autorepeat = ((lparam & 0xff) != 0);
			app->onCharcode(key,
				pressed,
				autorepeat);
		}
		break;

		case WM_SYSCOMMAND:
		{
			if (wparam == SC_MINIMIZE && !m_suspended)
			{
				m_suspended = true;
				app->onSuspend();
			}
			else if (wparam == SC_RESTORE && m_suspended)
			{
				m_suspended = false;
				app->onRestore();
			}
		}
		break;
		}

		return DefWindowProc(hwnd,
			message,
			wparam,
			lparam);
	}
}

#endif