//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

namespace RoxRender
{

struct Statistics
{
public:
    static void beginFrame();
    static Statistics &get();

public:
    unsigned int draw_count;
    unsigned int verts_count;
    unsigned int opaque_poly_count;
    unsigned int transparent_poly_count;

    Statistics(): draw_count(0),verts_count(0),opaque_poly_count(0),transparent_poly_count(0) {}

public:
    static bool enabled();
};

}
