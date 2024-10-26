// Updated by the Rox-engine
// Copyright © 2024 Torox Project
//
// This file is part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License: for non-commercial and commercial use under specific conditions.
// 2. Commercial License: for use on proprietary platforms.
// 
// For full licensing terms, please refer to the LICENSE file in the root directory of this project.

#include "RoxApp.h"
#include "RoxPlatformFactory.h"

#include <stdexcept>
#include <string>

#ifndef __APPLE__ //implemented in app.mm

namespace RoxApp
{
	RoxApp::RoxApp()
	{
		platform_adapter = createPlatformAdapter();
		if (!platform_adapter)
		{
			throw std::runtime_error("Unsupported platform");
		}
	}

	RoxApp::~RoxApp()
	{
		delete platform_adapter;
	}

	void RoxApp::startWindowed(int x,
	                           int y,
	                           unsigned int w,
	                           unsigned int h,
	                           int antialiasing)
	{
		platform_adapter->startWindowed(x,
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
		platform_adapter->startFullscreen(w,
		                                  h,
		                                  aa,
		                                  *this);
	}

	void RoxApp::setTitle(const char* title)
	{
		platform_adapter->setTitle(title);
	}

	std::string RoxApp::getTitle()
	{
		return platform_adapter->getTitle();
	}

	void RoxApp::setVirtualKeyboard(::RoxInput::VIRTUAL_KEYBOARD_TYPE type)
	{
		platform_adapter->setVirtualKeyboard(type);
	}

	void RoxApp::setMousePos(int x,
	                         int y)
	{
	}

	void RoxApp::finish()
	{
		platform_adapter->finish(*this);
	}
}

#endif
