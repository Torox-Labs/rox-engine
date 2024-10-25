// RoxWindowsAdapter.h
#pragma once

#include "IRoxPlatformAdapter.h"
#include "RoxApp.h"
#include <string>

#include <windows.h>
#include <windowsx.h>

#ifndef DIRECTX11
#include <gl/gl.h>
#include <gl/wglext.h>
#include <gl/glext.h>
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
        HDC m_hdc;
        HGLRC m_hglrc;
#endif
        std::string m_title;
        unsigned long m_time;
        static bool m_suspended;

        // Private helper functions
        static unsigned int get_x11_key(unsigned int key);
        static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    };
}
