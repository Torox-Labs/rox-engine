﻿// Updated by the Rox-engine
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
#include <glad/include/glad/glad.h>
#include "RoxRender/RoxRenderOpengl.h"
#endif

using uint = unsigned int;

#pragma region From <glext.h>
#define GL_MULTISAMPLE_ARB                0x809D
#pragma endregion


#pragma region From <wglext.h>

#define WGL_CONTEXT_MAJOR_VERSION_ARB		0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB		0x2092
#define WGL_CONTEXT_FLAGS_ARB				0x2094
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB	0x00000001
#define WGL_CONTEXT_PROFILE_MASK_ARB		0x9126

//
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_ACCELERATION_ARB              0x2003
#define WGL_FULL_ACCELERATION_ARB         0x2027
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_ALPHA_BITS_ARB                0x201B
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_SAMPLE_BUFFERS_ARB            0x2041
#define WGL_SAMPLES_ARB                   0x2042

// Declare Create Context Attribs function pointer
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(
	HDC,      			 // First parameter: Device Context
	HGLRC,				// Second parameter: Share Context
	const int* attribs // Third parameter: Context Attributes
	);
typedef BOOL(WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);


#pragma endregion

#pragma region From <wgl.h>

// Extension string query function pointer type
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGEXTPROC) (void);
// VSync control function piointer types
typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC) (int);
typedef int (WINAPI* PFNWGLGETSWAPINTERVALEXTPROC) (void);

// Check if extension listed
typedef const char* (WINAPI* PFNWGLGETEXTENSIONSSTRINGARBPROC) (HDC hdc);

#pragma endregion

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
		m_hWnd(nullptr), m_title("Rox engine"), m_time(0)
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
		RoxLogger::log() << "Windows Creation\n";
		// Set the pointer to the RoxApp object
		m_app = &app;

		// Initialize the window
		m_instance = GetModuleHandle(nullptr);
		if (!m_instance)
			return;

		const char  CLASS_NAME[] = "rox_engine";


		// Register the window class
		WNDCLASS  wc = {};
		wc.hInstance = m_instance;
		wc.lpfnWndProc = RoxWindowsAdapter::wnd_proc;
		wc.lpszClassName = CLASS_NAME;

		if (!RegisterClass(&wc))
		{
			RoxSystem::log() << "Failed to register window class. Error code: " << GetLastError() << "\n";
			return;
		}

		//Create Window Style
		// To make the window resizable add WS_THICKFRAME
		DWORD style = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

		// Client Area Rectangle
		RECT windowRect = {
			x,
			y,
			static_cast<int>(x + w),
			static_cast<int>(y + h)
		};

		// Create the main window
		AdjustWindowRect(&windowRect, style, false);
		m_hWnd = CreateWindowEx(
			0,
			CLASS_NAME,
			m_title.c_str(),
			style,
			
			windowRect.left,
			windowRect.top,
			windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			
			NULL,
			NULL,
			m_instance,
			NULL);

		if (!m_hWnd)
		{
			DWORD err = GetLastError();
			RoxSystem::log() << "Failed to create window. Error code: " << err << "\n";
			return;
		}

		// Store Reference to the window Device Context
		m_hdc = GetDC(m_hWnd);


		// Pixel Format
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32; // Original 24
		pfd.cAlphaBits = 8;
		pfd.cDepthBits = 32; // Original 24
		pfd.cStencilBits = 8;
		pfd.iLayerType = PFD_MAIN_PLANE;
		int pixelFormat = ChoosePixelFormat(m_hdc, &pfd);

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
		sd.OutputWindow = m_hWnd;
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

		if (!pixelFormat)
		{
			RoxSystem::log() << "Failed to create Pixel Format.\n";
			return;
		}

		if (!SetPixelFormat(m_hdc, pixelFormat, &pfd))
			return;

		// -----------------// OpenGL //----------------------
		// Create temporary OpenGL context
		m_tempRC = wglCreateContext(m_hdc);
		if (!m_tempRC)
		{
			RoxSystem::log() << "Failed to create Temp Rendering Context.\n";
			return;
		}

		wglMakeCurrent(m_hdc, m_tempRC);

		// Load the wglCreateContextAttribsARB function
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

		// Define attributes for OpenGL 3.3 core profile context
		const int attribList[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
			WGL_CONTEXT_MINOR_VERSION_ARB, 3,
			WGL_CONTEXT_FLAGS_ARB, 0,
			WGL_CONTEXT_PROFILE_MASK_ARB,
			WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0, // Terminate the list
		};

		// Create modern OpenGL context
		m_hglrc = wglCreateContextAttribsARB(m_hdc, nullptr, attribList);

		// Cleanup the temporary context
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(m_tempRC);

		// Assigning the modern OpenGL
		wglMakeCurrent(m_hdc, m_hglrc);

		// Load GLAD
		if (!gladLoadGL())
		{
			RoxLogger::log() << "Couldn't Load GLAD \n";
		}
		else
		{
			RoxLogger::log() << "OpenGL Version " << GLVersion.major << "." << GLVersion.minor << "\n";
		}

		// Enable Vertical Synchronization using vSynch Extension
		PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)wglGetProcAddress("wglGetExtensionsStringEXT");
		bool swapControlSupported = strstr(_wglGetExtensionsStringEXT(), "WGL_EXT_swap_control") != 0;

		int vSync = 0;
		if (swapControlSupported)
		{
			PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
			PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");
			if (wglSwapIntervalEXT(1))
			{
				RoxLogger::log() << "Enabled vSynch\n";
				vSync = wglGetSwapIntervalEXT();
			}
			else
			{
				RoxLogger::log() << "Couldn't enable vSynch\n";
			}
		}
		else
		{
			RoxLogger::log() << "vSynch (WGL_EXT_swap_control) not supported\n";
		}


		// Show the window
		ShowWindow(m_hWnd, SW_SHOW);

		PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
		if (antialiasing > 0)
		{
			wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));

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

			ReleaseDC(m_hWnd, m_hdc);
			DestroyWindow(m_hWnd);

			m_hWnd = CreateWindowA(
				"rox_engine",
				m_title
				.c_str(),
				style,
				
				windowRect.left,
				windowRect.top,
				windowRect.right - windowRect.left,
				windowRect.bottom - windowRect.top,

				NULL,
				NULL,
				m_instance,
				NULL);

			ShowWindow(m_hWnd,
				SW_SHOW);
			m_hdc = GetDC(m_hWnd);

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

		SetWindowTextA(m_hWnd,
			m_title
			.c_str());

		// Set the user data
		SetWindowLongPtr(m_hWnd,
			GWLP_USERDATA,
			(LONG_PTR)this);
		if(!SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this))
		{
			RoxSystem::log() << "Failed to set window user data\n";
			return;
		}

		// Initialize the application
		RoxRender::setViewport(0, 0, w, h);

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
		while (m_hWnd)
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

		if (m_hWnd)
			SetWindowTextA(m_hWnd, title);
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
		if (m_hWnd)
			SetCursorPos(x, y);
	}

	void RoxWindowsAdapter::finish(RoxApp& app)
	{
		if (!m_hWnd)
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
		ReleaseDC(m_hWnd,
			m_hdc);
		DestroyWindow(m_hWnd);

#endif

		m_hWnd = nullptr;
	}

	bool RoxWindowsAdapter::hasWGLExtension(const char* extension)
	{
		PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
			reinterpret_cast<PFNWGLGETEXTENSIONSSTRINGARBPROC>(
				wglGetProcAddress("wglGetExtensionsStringARB"));
		if (!wglGetExtensionsStringARB)
			return false;

		const char* extensions = wglGetExtensionsStringARB(m_hdc);
		return extensions && strstr(extensions, extension);
	}

	std::wstring RoxWindowsAdapter::stringToWString(const std::string& str)
	{
		 if (str.empty())
        return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), nullptr, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
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

	// Messages Listener
	LRESULT CALLBACK RoxWindowsAdapter::wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
	{
		RoxWindowsAdapter* pThis = (RoxWindowsAdapter*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (!pThis)
			return DefWindowProc(hwnd,
				message,
				wparam,
				lparam);


		if (hwnd == pThis->m_hWnd)
		{
			switch (message)
			{
			case WM_SIZE:
				break;

			case WM_CLOSE: getInstance()
					.finish(*pThis->m_app);
				break;

			case WM_MOUSEWHEEL:
				{
					const int WHEEL_DELTA_SCALE = 60;

					const int x = GET_X_LPARAM(wparam);
					const int y = GET_Y_LPARAM(wparam);

					pThis->m_app->onMouseScroll(x / WHEEL_DELTA_SCALE,
					                            y / WHEEL_DELTA_SCALE);
				}
				break;

			case WM_MOUSEMOVE:
				{
					const int x = LOWORD(lparam);
					const int y = HIWORD(lparam);

					POINT pt = { x, y };

					RECT rc;
					GetClientRect(hwnd,
					              &rc);

					pThis->m_app->onMouseMove(pt.x, pt.y);
				}
				break;

			case WM_LBUTTONDOWN: pThis->m_app->onMouseButton(RoxInput::MOUSE_LEFT,
			                                                 true);
				break;
			case WM_LBUTTONUP: pThis->m_app->onMouseButton(RoxInput::MOUSE_LEFT,
			                                               false);
				break;
			case WM_MBUTTONDOWN: pThis->m_app->onMouseButton(RoxInput::MOUSE_MIDDLE,
			                                                 true);
				break;
			case WM_MBUTTONUP: pThis->m_app->onMouseButton(RoxInput::MOUSE_MIDDLE,
			                                               false);
				break;
			case WM_RBUTTONDOWN: pThis->m_app->onMouseButton(RoxInput::MOUSE_RIGHT,
			                                                 true);
				break;
			case WM_RBUTTONUP: pThis->m_app->onMouseButton(RoxInput::MOUSE_RIGHT,
			                                               false);
				break;

			case WM_KEYDOWN:
				{
					const unsigned int key = LOWORD(wparam);
					const unsigned int x11key = get_x11_key(key);
					if (x11key)
						pThis->m_app->onKeyDown(x11key,
						                        true);
				}
				break;

			case WM_KEYUP:
				{
					const unsigned int key = LOWORD(wparam);
					const unsigned int x11key = get_x11_key(key);
					if (x11key)
						pThis->m_app->onKeyUp(x11key,
						                      false);
				}
				break;

			case WM_CHAR:
				{
					const unsigned int key = wparam;
					const bool pressed = ((lparam & (1 << 31)) == 0);
					const bool autorepeat = ((lparam & 0xff) != 0);
					pThis->m_app->onCharcode(key,
					                         pressed,
					                         autorepeat);
				}
				break;

			case WM_SYSCOMMAND:
				{
					if (wparam == SC_MINIMIZE && !m_suspended)
					{
						m_suspended = true;
						pThis->m_app->onSuspend();
					}
					else if (wparam == SC_RESTORE && m_suspended)
					{
						m_suspended = false;
						pThis->m_app->onRestore();
					}
				}
				break;
			}
		}

		return DefWindowProc(hwnd,
			message,
			wparam,
			lparam);
	}
}

#endif