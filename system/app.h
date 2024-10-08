//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "button_codes.h"

namespace nya_system
{

enum mouse_button
{
    mouse_left,
    mouse_middle,
    mouse_right
};

enum virtual_keyboard_type
{
    keyboard_hidden = 'h',
    keyboard_numeric = 'n',
    keyboard_decimal = 'd',
    keyboard_phone = 'f',
    keyboard_text = 't',
    keyboard_pin = 'p',
    keyboard_email = 'e',
    keyboard_password  = 'w',
    keyboard_url = 'u'
};

class app
{
public:
    virtual bool on_splash() { return false; } //shown if true
    virtual void on_init() {}
    virtual void on_frame(unsigned int dt) {}
    virtual void on_free() {}

public:
    virtual void on_suspend() {}
    virtual void on_restore() {}

public:
    virtual void on_mouse_move(int x,int y) {}
    virtual void on_mouse_button(mouse_button button,bool pressed) {}
    virtual void on_mouse_scroll(int dx,int dy) {}
    virtual void on_keyboard(unsigned int key,bool pressed) {}
    virtual void on_charcode(unsigned int key,bool pressed,bool autorepeat) {}
    virtual void on_touch(int x,int y,unsigned int touch_idx,bool pressed) {}
    virtual void on_acceleration(float x,float y, float z) {}

public:
    virtual void on_resize(unsigned int w,unsigned int h) {}

public:
    void start_windowed(int x,int y,unsigned int w,unsigned int h,int antialiasing);
    void start_fullscreen(unsigned int w,unsigned int h,int antialiasing);
    void set_title(const char *title);
    void set_virtual_keyboard(virtual_keyboard_type type);
    void set_mouse_pos(int x,int y);
    void finish();
};

}
