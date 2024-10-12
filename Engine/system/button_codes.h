//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace nya_system
{

enum button_code
{
    //X11 codes

    key_return    =0xff0d,
    key_escape    =0xff1b,
    key_space     =0x0020,
    key_tab       =0xff09,
    key_pause     =0xff13,

    key_shift     =0xffe1,
    key_shift_r   =0xffe2,
    key_control   =0xffe3,
    key_control_r =0xffe4,
    key_alt       =0xffe9,
    key_alt_r     =0xffea,
    key_capital   =0xffe5,

    key_up        =0xff52,
    key_down      =0xff54,
    key_left      =0xff51,
    key_right     =0xff53,

    key_page_up   =0xff55,
    key_page_down =0xff56,
    key_end       =0xff57,
    key_home      =0xff50,
    key_insert    =0xff63,
    key_delete    =0xffff,
    key_backspace =0xff08,
    
    key_a         =0x0061,
    key_b         =0x0062,
    key_c         =0x0063,
    key_d         =0x0064,
    key_e         =0x0065,
    key_f         =0x0066,
    key_g         =0x0067,
    key_h         =0x0068,
    key_i         =0x0069,
    key_j         =0x006a,
    key_k         =0x006b,
    key_l         =0x006c,
    key_m         =0x006d,
    key_n         =0x006e,
    key_o         =0x006f,
    key_p         =0x0070,
    key_q         =0x0071,
    key_r         =0x0072,
    key_s         =0x0073,
    key_t         =0x0074,
    key_u         =0x0075,
    key_v         =0x0076,
    key_w         =0x0077,
    key_x         =0x0078,
    key_y         =0x0079,
    key_z         =0x007a,

    key_0         =0x0030,
    key_1         =0x0031,
    key_2         =0x0032,
    key_3         =0x0033,
    key_4         =0x0034,
    key_5         =0x0035,
    key_6         =0x0036,
    key_7         =0x0037,
    key_8         =0x0038,
    key_9         =0x0039,

    key_f1        =0xffbe,
    key_f2        =0xffbf,
    key_f3        =0xffc0,
    key_f4        =0xffc1,
    key_f5        =0xffc2,
    key_f6        =0xffc3,
    key_f7        =0xffc4,
    key_f8        =0xffc5,
    key_f9        =0xffc6,
    key_f10       =0xffc7,
    key_f11       =0xffc8,
    key_f12       =0xffc9,
    
    key_comma     =0x002c,
    key_period    =0x002e,
    key_bracket_left =0x005b,
    key_bracket_right=0x005d,

    //additional codes

    key_back      =0xffff01,
};

}
