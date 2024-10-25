// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
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
