//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>
#include <string>
#include <stddef.h>
#include "math/vector.h"
#include "math/quaternion.h"

namespace nya_formats
{

struct nan
{
    unsigned int version;

    template <typename t> struct curve
    {
        std::string bone_name;
        std::vector<t> frames;
    };

    struct pos_vec3_linear_frame { unsigned int time; nya_math::vec3 pos; };
    std::vector<curve<pos_vec3_linear_frame> > pos_vec3_linear_curves;

    struct rot_quat_linear_frame { unsigned int time; nya_math::quat rot; };
    std::vector<curve<rot_quat_linear_frame> > rot_quat_linear_curves;

    struct float_linear_frame { unsigned int time; float value; };
    std::vector<curve<float_linear_frame> > float_linear_curves;

    template <typename t> static t &add_curve(std::vector<t> &curves,const std::string &name)
    {
        for(size_t i=0;i<curves.size();++i)
        {
            if(curves[i].bone_name==name)
                return curves[i];
        }

        curves.resize(curves.size()+1);
        curves.back().bone_name=name;
        return curves.back();
    }

    nan(): version(0) {}

public:
    bool read(const void *data,size_t size);
    size_t write(void *to_data,size_t to_size) const; //to_size=get_size()
    size_t get_size() const { return write(0,0); }
};

}
