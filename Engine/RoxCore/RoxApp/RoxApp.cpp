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

#include "RoxApp.h"
#include "RoxSystem/RoxSystem.h"
#include "RoxRender/RoxRender.h"

#include <string>

//MARK: Windows
#ifdef WIN32

#include "RoxWindowsAdapter.cpp";

#ifndef DIRECTX11
#include <gl/gl.h>
#include <gl/wglext.h>
#include <gl/glext.h>
#include "RoxRender/RoxRenderOpengl.h"
#endif


//MARK: Apple
#elif defined __APPLE__ //implemented in app.mm
#endif

#ifndef __APPLE__ //implemented in app.mm

// Todo: UPdate the code to work with the specific platform
namespace RoxApp
{
	void RoxApp::startWindowed(int x,
	                           int y,
	                           unsigned int w,
	                           unsigned int h,
	                           int antialiasing)
	{
		RoxWindowsAdapter::getApp()
			.startWindowed(x,
			               y,
			               w,
			               h,
			               antialiasing,
			               *this);
	}

	void RoxApp::startFullscreen(unsigned int w,
	                             unsigned int h,
	                             int aa)
	{
		RoxWindowsAdapter::getApp()
			.startFullscreen(w,
			                 h,
			                 aa,
			                 *this);
	}

	void RoxApp::setTitle(const char* title)
	{
		RoxWindowsAdapter::getApp()
			.setTitle(title);
	}

	std::string RoxApp::getTitle()
	{
		return RoxWindowsAdapter::getApp()
			.getTitle();
	}

	void RoxApp::setVirtualKeyboard(::RoxInput::VIRTUAL_KEYBOARD_TYPE type)
	{
		RoxWindowsAdapter::getApp()
			.setVirtualKeyboard(type);
	}

	void RoxApp::setMousePos(int x,
	                         int y)
	{
	}

	void RoxApp::finish()
	{
		RoxWindowsAdapter::getApp()
			.finish(*this);
	}
}

#endif
