//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "string_convert.h"
#include <algorithm>
#include <sstream>
#include <string.h>

namespace nya_formats
{

inline std::string fix_string(const std::string &s,const std::string whitespaces = " \t\r\n")
{
    std::string ss=s;
    std::transform(ss.begin(),ss.end(),ss.begin(),::tolower);
    const size_t start_idx=ss.find_first_not_of(whitespaces);
    if(start_idx==std::string::npos)
        return std::string("");

    const size_t end_idx=ss.find_last_not_of(whitespaces)+1;
    return ss.substr(start_idx,end_idx-start_idx);
}

bool bool_from_string(const char *s)
{
    if(!s)
        return false;

    return strchr("YyTt1",s[0]) != 0;
}

nya_math::vec4 vec4_from_string(const char *s)
{
    nya_math::vec4 v;
    std::string ss=s;
    std::replace(ss.begin(),ss.end(),',',' ');
    std::istringstream iss(ss);
    if(iss>>v.x)
        if(iss>>v.y)
            if(iss>>v.z)
                iss>>v.w;
    return v;
}

nya_render::blend::mode blend_mode_from_string(const char *s)
{
    if(!s)
        return nya_render::blend::one;

    const std::string ss=fix_string(s);
    if(ss=="src_alpha") return nya_render::blend::src_alpha;
    if(ss=="inv_src_alpha") return nya_render::blend::inv_src_alpha;
    if(ss=="src_color") return nya_render::blend::src_color;
    if(ss=="inv_src_color") return nya_render::blend::inv_src_color;
    if(ss=="dst_color") return nya_render::blend::dst_color;
    if(ss=="inv_dst_color") return nya_render::blend::inv_dst_color;
    if(ss=="dst_alpha") return nya_render::blend::dst_alpha;
    if(ss=="inv_dst_alpha") return nya_render::blend::inv_dst_alpha;
    if(ss=="zero") return nya_render::blend::zero;
    if(ss=="one") return nya_render::blend::one;

    return nya_render::blend::one;
}

bool blend_mode_from_string(const char *s,nya_render::blend::mode &src_out,nya_render::blend::mode &dst_out)
{
    if(!s)
        return false;

    std::string str(s);

    const size_t div_idx=str.find(':');
    if(div_idx==std::string::npos)
    {
        src_out=nya_render::blend::one;
        dst_out=nya_render::blend::zero;
        return false;
    }

    dst_out=blend_mode_from_string(str.substr(div_idx+1).c_str());
    str.resize(div_idx);
    src_out=blend_mode_from_string(str.c_str());
    return true;
}

bool cull_face_from_string(const char *s,nya_render::cull_face::order &order_out)
{
    if(!s)
        return false;

    const std::string ss=fix_string(s);
    if(ss=="cw")
    {
        order_out=nya_render::cull_face::cw;
        return true;
    }

    if(ss=="ccw")
    {
        order_out=nya_render::cull_face::ccw;
        return true;
    }

    order_out=nya_render::cull_face::ccw;
    return false;
}

}
