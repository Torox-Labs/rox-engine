// Updated by the Rox-engine
// Copyright © 2024 Torox Project
//
// This file is part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License: for non-commercial and commercial use under specific conditions.
// 2. Commercial License: for use on proprietary platforms.
// 
// For full licensing terms, please refer to the LICENSE file in the root directory of this project.

#pragma once

#include <string>
#include "RoxButtonCodes.h"

namespace RoxInput
{
	enum MOUSE_BOTTON
	{
		MOUSE_LEFT,
		MOUSE_MIDDLE,
		MOUSE_RIGHT
	};

	enum VIRTUAL_KEYBOARD_TYPE
	{
		KEYBOARD_HIDDEN = 'h',
		KEYBOARD_NUMERIC = 'n',
		KEYBOARD_DECIMAL = 'd',
		KEYBOARD_PHONE = 'f',
		KEYBOARD_TEXT = 't',
		KEYBOARD_PIN = 'p',
		KEYBOARD_EMAIL = 'e',
		KEYBOARD_PASSWORD = 'w',
		KEYBOARD_URL = 'u'
	};

	class RoxInput
	{
	/*public:
		RoxInput();
		virtual ~RoxInput();*/

	public:
		virtual void onMouseMove(int x, int y) {}
		virtual void onMouseButton(MOUSE_BOTTON button, bool pressed) {}
		virtual void onMouseScroll(int dx, int dy) {}
		virtual void onKeyDown(unsigned int key, bool pressed) {}
		virtual void onKeyUp(unsigned int key, bool pressed) {}
		virtual void onCharcode(unsigned int key, bool pressed, bool autorepeat) {}
		virtual void onTouch(int x, int y, unsigned int touch_idx, bool pressed) {}
		virtual void onAcceleration(float x, float y, float z) {}
	};
}
