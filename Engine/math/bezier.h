//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace nya_math
{

class bezier
{
public:
    float get(float x) const;

public:
    bezier(): m_linear(true) {}
    bezier(float x1,float y1,float x2,float y2);

private:
    float get_y(float x,float x1,float y1,float x2,float y2);

private:
    static const int div_count=16;
    float m_y[div_count+1];

    bool m_linear;
};

}
