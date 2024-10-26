/// Updated by the Rox-engine
// Copyright © 2024 Torox Project
//
// This file is part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License: for non-commercial and commercial use under specific conditions.
// 2. Commercial License: for use on proprietary platforms.
// 
// For full licensing terms, please refer to the LICENSE file in the root directory of this project.

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