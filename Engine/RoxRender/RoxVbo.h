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

namespace nya_memory { class tmp_buffer_ref; }

namespace RoxRender
{

class RoxVbo
{
public:
	enum ELEMENT_TYPE
    {
        TRIANGLES,
        TRIANGLE_STRIP,
        POINTS,
        LINES,
        LINE_STRIP
    };

    enum INDEX_SIZE
    {
        INDEX_2D = 2,
		INDEX_4D = 4
    };

    enum VERTEX_ATRIB_TYPE
    {
        FLOAT_16,
        FLOAT_32,
        UINT_8
    };

    enum USAGE_HINT
    {
        STATIC_DRAW,
        DYNAMIC_DRAW,
        STREAM_DRAW
    };

    typedef unsigned int uint;

    const static uint max_tex_coord=13;

    struct Layout
    {
        struct attribute { unsigned char offset,dimension; VERTEX_ATRIB_TYPE type; attribute():offset(0),dimension(0),type(FLOAT_32){} };
        attribute pos,normal,color;
        attribute tc[max_tex_coord];

        Layout() { pos.dimension=3; }
    };

public:
    bool setVertexData(const void*data,uint vert_stride,uint vert_count, USAGE_HINT usage=STATIC_DRAW);
    bool setIndexData(const void*data,INDEX_SIZE size,uint indices_count, USAGE_HINT usage=STATIC_DRAW);
    void setElementType(ELEMENT_TYPE type);
    void setVertices(uint offset,uint dimension,VERTEX_ATRIB_TYPE=FLOAT_32);
    void setNormals(uint offset,VERTEX_ATRIB_TYPE=FLOAT_32);
    void setTc(uint tc_idx,uint offset,uint dimension,VERTEX_ATRIB_TYPE=FLOAT_32);
    void setColors(uint offset,uint dimension,VERTEX_ATRIB_TYPE=FLOAT_32);
    void setLayout(const Layout&l);

public:
    bool getVertexData(nya_memory::tmp_buffer_ref &data) const;
    bool getIndexData(nya_memory::tmp_buffer_ref &data) const;
    ELEMENT_TYPE getElementType() const;
    uint getVertsCount() const;
    uint getVertStride() const;
    uint getVertOffset() const;
    uint getVertDimension() const;
    uint getNormalsOffset() const;
    uint getTcOffset(uint idx) const;
    uint getTcDimension(uint idx) const;
    uint getColorsOffset() const;
    uint getColorsDimension() const;
    uint getIndicesCount() const;
    INDEX_SIZE getIndexSize() const;
    const Layout&getLayout() const;

public:
    void bind() const { bindVerts(); bindIndices(); }

    void bindVerts() const;
    void bindIndices() const;

    static void unbind();

public:
    static void draw();
    static void draw(uint count);
    static void draw(uint offset,uint count,ELEMENT_TYPE type=TRIANGLES,uint instances=1);
    static void transformFeedback(RoxVbo&target);
    static void transformFeedback(RoxVbo&target,uint src_offset,uint dst_offset,uint count,ELEMENT_TYPE type= POINTS);

public:
    void release();

private:
    void releaseVerts();
    void releaseIndices();

public:
    static uint getUsedVmemSize();
    
public:
    static bool isTransformFeedbackSupported();

public:
    RoxVbo(): m_verts(-1),m_indices(-1),m_vert_count(0),m_ind_count(0),
           m_stride(0),m_ind_size(INDEX_2D),m_ElementType(TRIANGLES) {}

private:
    int m_verts;
    int m_indices;
    int m_vert_count;
    int m_ind_count;
    Layout m_layout;
    int m_stride;
    INDEX_SIZE m_ind_size;
    ELEMENT_TYPE m_ElementType;
};

}
