#pragma once

#include <string>
#include "RoxInput/RoxInput.h"

// Forward declaration of RoxApp class
namespace RoxApp
{
    class RoxApp;
}

namespace RoxApp
{
    class IRoxPlatformAdapter
    {
    public:
        virtual void startWindowed(int x, int y, unsigned int w, unsigned int h, int antialiasing, RoxApp& app) = 0;
        virtual void startFullscreen(unsigned int w, unsigned int h, int antialiasing, RoxApp& app) = 0;
        virtual void setTitle(const char* title) = 0;
        virtual std::string getTitle() = 0;
        virtual void setVirtualKeyboard(::RoxInput::VIRTUAL_KEYBOARD_TYPE type) = 0;
        virtual void setMousePos(int x, int y) = 0;
        virtual void finish(RoxApp& app) = 0;

        virtual ~IRoxPlatformAdapter() = default;
    };
}
