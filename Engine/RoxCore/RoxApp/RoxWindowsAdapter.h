// Updated by the Rox-engine
// Copyright © 2024 Torox Project
//
// This file is part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License: for non-commercial and commercial use under specific conditions.
// 2. Commercial License: for use on proprietary platforms.
// 
// For full licensing terms, please refer to the LICENSE file in the root directory of this project.

#pragma once

#include "IRoxPlatformAdapter.h"
#include "RoxApp.h"
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>

#ifndef DIRECTX11
#include "RoxRender/RoxRenderOpengl.h"
#endif

namespace RoxApp
{
    class RoxWindowsAdapter : public IRoxPlatformAdapter
    {
    public:
        RoxWindowsAdapter();
        ~RoxWindowsAdapter();

        static RoxWindowsAdapter& getInstance();

        void startWindowed(int x, int y, unsigned int w, unsigned int h, int antialiasing, RoxApp& app) override;
        void startFullscreen(unsigned int w, unsigned int h, int antialiasing, RoxApp& app) override;
        void setTitle(const char* title) override;
        std::string getTitle() override;
        void setVirtualKeyboard(::RoxInput::VIRTUAL_KEYBOARD_TYPE type) override;
        void setMousePos(int x, int y) override;
        void finish(RoxApp& app) override;

    private:
        // Member variables
        HINSTANCE m_instance;
        HWND m_hwnd;
#ifdef DIRECTX11
        // DirectX-specific members
#else
		HDC m_hdc; // Parent Window Device Context
		HWND m_hwndChild; // Child Window Device Context
		HGLRC m_hglrc; // OpenGL Rendering Context
#endif
        std::string m_title;
        unsigned long m_time;
        static bool m_suspended;

        // Private helper functions
        static unsigned int get_x11_key(unsigned int key);
        static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    };
}

#endif