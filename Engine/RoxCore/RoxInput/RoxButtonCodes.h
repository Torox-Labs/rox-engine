// Copyright © 2024 Torox Project
// Portions Copyright © 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
//
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

namespace RoxInput
{

enum ButtonCode
{
    //X11 codes
    KEY_RETURN    = 0xff0d,
    KEY_ESCAPE    = 0xff1b,
    KEY_SPACE     = 0x0020,
    KEY_TAB       = 0xff09,
    KEY_PAUSE     = 0xff13,

    KEY_SHIFT     = 0xffe1,
    KEY_SHIFT_R   = 0xffe2,
    KEY_CONTROL   = 0xffe3,
    KEY_CONTROL_R = 0xffe4,
    KEY_ALT       = 0xffe9,
    KEY_ALT_R     = 0xffea,
    KEY_CAPITAL   = 0xffe5,

    KEY_UP        = 0xff52,
    KEY_DOWN      = 0xff54,
    KEY_LEFT      = 0xff51,
    KEY_RIGHT     = 0xff53,

    KEY_PAGE_UP   = 0xff55,
    KEY_PAGE_DOWN = 0xff56,
    KEY_END       = 0xff57,
    KEY_HOME      = 0xff50,
    KEY_INSERT    = 0xff63,
    KEY_DELETE    = 0xffff,
    KEY_BACKSPACE = 0xff08,
    
    KEY_A         = 0x0061,
    KEY_B         = 0x0062,
    KEY_C         = 0x0063,
    KEY_D         = 0x0064,
    KEY_E         = 0x0065,
    KEY_F         = 0x0066,
    KEY_G         = 0x0067,
    KEY_H         = 0x0068,
    KEY_I         = 0x0069,
    KEY_J         = 0x006a,
    KEY_K         = 0x006b,
    KEY_L         = 0x006c,
    KEY_M         = 0x006d,
    KEY_N         = 0x006e,
    KEY_O         = 0x006f,
    KEY_P         = 0x0070,
    KEY_Q         = 0x0071,
    KEY_R         = 0x0072,
    KEY_S         = 0x0073,
    KEY_T         = 0x0074,
    KEY_U         = 0x0075,
    KEY_V         = 0x0076,
    KEY_W         = 0x0077,
    KEY_X         = 0x0078,
    KEY_Y         = 0x0079,
    KEY_Z         = 0x007a,

    KEY_0         = 0x0030,
    KEY_1         = 0x0031,
    KEY_2         = 0x0032,
    KEY_3         = 0x0033,
    KEY_4         = 0x0034,
    KEY_5         = 0x0035,
    KEY_6         = 0x0036,
    KEY_7         = 0x0037,
    KEY_8         = 0x0038,
    KEY_9         = 0x0039,

    KEY_F1        = 0xffbe,
    KEY_F2        = 0xffbf,
    KEY_F3        = 0xffc0,
    KEY_F4        = 0xffc1,
    KEY_F5        = 0xffc2,
    KEY_F6        = 0xffc3,
    KEY_F7        = 0xffc4,
    KEY_F8        = 0xffc5,
    KEY_F9        = 0xffc6,
    KEY_F10       = 0xffc7,
    KEY_F11       = 0xffc8,
    KEY_F12       = 0xffc9,

    KEY_COMMA     = 0x002c,
    KEY_PERIOD    = 0x002e,
    KEY_BRACKET_LEFT = 0x005b,
    KEY_BRACKET_RIGHT = 0x005d,

    // additional codes
    KEY_BACK      = 0xffff01,
};

}
