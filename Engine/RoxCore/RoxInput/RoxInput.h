// rox_engine.h
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
	public:
		RoxInput();
		virtual ~RoxInput();

	public:
		virtual void onMouseMove(int x, int y) {}
		virtual void onMouseButton(MOUSE_BOTTON button, bool pressed) {}
		virtual void onMouseScroll(int dx, int dy) {}
		virtual void onKeyboard(unsigned int key, bool pressed) {}
		virtual void onCharcode(unsigned int key, bool pressed, bool autorepeat) {}
		virtual void onTouch(int x, int y, unsigned int touch_idx, bool pressed) {}
		virtual void onAcceleration(float x, float y, float z) {}
	};
}
