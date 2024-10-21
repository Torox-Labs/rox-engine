//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxRenderBuffered.h"

namespace RoxRender
{

    bool RoxRenderBuffered::getVertexData(int idx, void* data)
    {
        log() << "get_vertex_data is not supported in RoxRenderBuffered\n";
        return false;
    }

    bool RoxRenderBuffered::getIndexData(int idx, void* data)
    {
        log() << "get_index_data is not supported in RoxRenderBuffered\n";
        return false;
    }

    bool RoxRenderBuffered::getTextureData(int RoxTexture, uint x, uint y, uint w, uint h, void* data)
    {
        log() << "get_texture_data is not supported in RoxRenderBuffered\n";
        return false;
    }

    //----------------------------------------------------------------

    void RoxRenderBuffered::setUniform(int uniform_buffer, int idx, const float* buf, uint count)
    {
        uniform_data d;
        d.buf_idx = uniform_buffer, d.idx = idx, d.count = count;
        m_current.write(CMD_UNIFORM, d, count, buf);
    }

    void RoxRenderBuffered::setCamera(const RoxMath::Matrix4& modelview, const RoxMath::Matrix4& projection)
    {
        camera_data d;
        d.mv = modelview;
        d.p = projection;
        m_current.write(CMD_CAMERA, d);
    }

    void RoxRenderBuffered::clear(const ViewportState& s, bool color, bool depth, bool stencil)
    {
        clear_data d;
        d.vp = s, d.color = color, d.depth = depth, d.stencil = stencil;
        m_current.write(CMD_CLEAR, d);
    }

    void RoxRenderBuffered::draw(const State& s) { m_current.write(CMD_DRAW, s); }
    void RoxRenderBuffered::applyState(const State& s) { m_current.write(CMD_APPLY, s); }
    void RoxRenderBuffered::resolveTarget(int idx) { m_current.write(CMD_RESOLVE, idx); }

    int RoxRenderBuffered::createShader(const char* vertex, const char* fragment)
    {
        //ToDo
        std::vector<RoxShader::Uniform> uniforms;
        for (int i = 0; i < 2; ++i)
        {
            RoxShaderCodeParser p(!i ? vertex : fragment);
            for (int i = 0; i < p.getUniformsCount(); ++i)
            {
                const RoxShaderCodeParser::variable& v = p.getUniform(i);
                if (v.type == RoxShaderCodeParser::TYPE_MATRIX4 &&
                    (v.name == "_nya_ModelViewMatrix" || v.name == "_nya_ProjectionMatrix" || v.name == "_nya_ModelViewProjectionMatrix"))
                    continue;

                bool found = false;
                for (int j = 0; j < (int)uniforms.size(); ++j)
                {
                    if (uniforms[j].name == v.name)
                    {
                        found = true;
                        break;
                    }
                }
                if (found)
                    continue;

                RoxShader::Uniform u;
                u.name = v.name;
                u.type = (RoxShader::UNIFORM_TYPE)v.type;
                u.array_size = v.array_size;
                uniforms.push_back(u);
            }
        }

        shader_create_data d;
        d.idx = newIdx();
        d.vs_size = (int)strlen(vertex) + 1;
        d.ps_size = (int)strlen(fragment) + 1;

        m_uniform_info[d.idx] = uniforms;

        m_current.write(CMD_SHDR_CREATE, d);
        m_current.write(d.vs_size, vertex);
        m_current.write(d.ps_size, fragment);
        return d.idx;
    }

    RoxRenderBuffered::uint RoxRenderBuffered::getUniformsCount(int RoxShader) { return (int)m_uniform_info[RoxShader].size(); }
    RoxShader::Uniform RoxRenderBuffered::getUniform(int RoxShader, int idx) { return m_uniform_info[RoxShader][idx]; }
    void RoxRenderBuffered::removeShader(int RoxShader) { m_current.write(CMD_SHDR_REMOVE, RoxShader); }

    int RoxRenderBuffered::createUniformBuffer(int RoxShader)
    {
        ubuf_create_data d;
        d.idx = newIdx();
        d.shader_idx = RoxShader;
        m_current.write(CMD_UBUF_CREATE, d);
        return d.idx;
    }

    void RoxRenderBuffered::removeUniformBuffer(int uniform_buffer) { m_current.write(CMD_UBUF_REMOVE, uniform_buffer); }

    int RoxRenderBuffered::createVertexBuffer(const void* data, uint stride, uint count, RoxVbo::USAGE_HINT usage)
    {
        vbuf_create_data d;
        d.idx = newIdx();
        d.stride = stride;
        d.count = count;
        d.usage = usage;
        m_current.write(CMD_VBUF_CREATE, d);
        m_current.write(stride * count, data);
        m_buf_sizes[d.idx] = stride * count;
        return d.idx;
    }

    void RoxRenderBuffered::setVertexLayout(int idx, RoxVbo::Layout Layout)
    {
        vbuf_layout d;
        d.idx = idx;
        d.layout = Layout;
        m_current.write(CMD_VBUF_LAYOUT, d);
    }

    void RoxRenderBuffered::updateVertexBuffer(int idx, const void* data)
    {
        buf_update d;
        d.idx = idx;
        d.size = m_buf_sizes[idx];
        m_current.write(CMD_VBUF_UPDATE, d);
        m_current.write(d.size, data);
    }

    void RoxRenderBuffered::removeVertexBuffer(int idx) { m_current.write(CMD_VBUF_REMOVE, idx); }

    int RoxRenderBuffered::createIndexBuffer(const void* data, RoxVbo::INDEX_SIZE type, uint count, RoxVbo::USAGE_HINT usage)
    {
        ibuf_create_data d;
        d.idx = newIdx();
        d.type = type;
        d.count = count;
        m_current.write(CMD_IBUF_CREATE, d);
        m_current.write(type * count, data);
        m_buf_sizes[d.idx] = type * count;
        return d.idx;
    }

    void RoxRenderBuffered::updateIndexBuffer(int idx, const void* data)
    {
        buf_update d;
        d.idx = idx;
        d.size = m_buf_sizes[idx];
        m_current.write(CMD_IBUF_UPDATE, d);
        m_current.write(d.size, data);
    }

    void RoxRenderBuffered::removeIndexBuffer(int idx) { m_current.write(CMD_IBUF_REMOVE, idx); }

    const int texture_size(unsigned int width, unsigned int height, RoxTexture::COLOR_FORMAT& format, int mip_count)
    {
        if (!mip_count)
            return 0;

        if (mip_count < 0)
            mip_count = 1;

        const bool is_pvrtc = format == RoxTexture::PVR_RGB2B || format == RoxTexture::PVR_RGB2B || format == RoxTexture::PVR_RGB4B || format == RoxTexture::PVR_RGBA4B;
        const unsigned int bpp = RoxTexture::getFormatBpp(format);
        unsigned int size = 0;

        for (unsigned int i = 0, w = width, h = height; i < (unsigned int)(mip_count > 0 ? mip_count : 1); ++i, w = w > 1 ? w / 2 : 1, h = h > 1 ? h / 2 : 1)
        {
            if (format < RoxTexture::DXT1)
                size += w * h * bpp / 8;
            else
            {
                if (is_pvrtc)
                {
                    if (format == RoxTexture::PVR_RGB2B || format == RoxTexture::PVR_RGB2B)
                        size += ((w > 16 ? w : 16) * (h > 8 ? h : 8) * 2 + 7) / 8;
                    else
                        size += ((w > 8 ? w : 8) * (h > 8 ? h : 8) * 4 + 7) / 8;
                }
                else
                    size += (w > 4 ? w : 4) / 4 * (h > 4 ? h : 4) / 4 * bpp * 2;
            }
        }

        return size;
    }

    int RoxRenderBuffered::createTexture(const void* data, uint width, uint height, RoxTexture::COLOR_FORMAT& format, int mip_count)
    {
        //ToDo: backend may modify format

        tex_create_data d;
        d.idx = newIdx();
        d.width = width;
        d.height = height;
        d.size = texture_size(width, height, format, mip_count);
        d.format = format;
        d.mip_count = mip_count;

        m_current.write(CMD_TEX_CREATE, d);
        if (d.size > 0)
            m_current.write(d.size, data);

        m_buf_sizes[d.idx] = RoxTexture::getFormatBpp(format);;
        return d.idx;
    }

    int RoxRenderBuffered::createCubemap(const void* data[6], uint width, RoxTexture::COLOR_FORMAT& format, int mip_count)
    {
        //ToDo: backend may modify format

        tex_create_data d;
        d.idx = newIdx();
        d.width = width;
        d.height = width;
        d.size = texture_size(width, width, format, mip_count);
        d.format = format;
        d.mip_count = mip_count;

        m_current.write(CMD_TEX_CUBE, d);
        if (d.size > 0)
        {
            for (int i = 0; i < 6; ++i)
                m_current.write(d.size, (char*)data[i]);
        }

        m_buf_sizes[d.idx] = RoxTexture::getFormatBpp(format);;
        return d.idx;
    }

    void RoxRenderBuffered::updateTexture(int idx, const void* data, uint x, uint y, uint width, uint height, int mip)
    {
        tex_update d;
        d.idx = idx;
        d.x = x, d.y = y, d.width = width, d.height = height;
        const uint bpp = m_buf_sizes[idx];
        for (uint i = 0, w = width, h = height; i < uint(mip > 0 ? mip : 1); ++i, w = w > 1 ? w / 2 : 1, h = h > 1 ? h / 2 : 1)
            d.size = w * h * bpp / 8;
        d.mip = mip;

        m_current.write(CMD_TEX_UPDATE, d);
        m_current.write(d.size, data);
    }

    void RoxRenderBuffered::setTextureWrap(int idx, RoxTexture::WRAP s, RoxTexture::WRAP t)
    {
        tex_wrap d;
        d.idx = idx;
        d.s = s, d.t = t;
        m_current.write(CMD_TEX_WRAP, d);
    }

    void RoxRenderBuffered::setTextureFilter(int idx, RoxTexture::FILTER minification, RoxTexture::FILTER magnification, RoxTexture::FILTER mipmap, uint aniso)
    {
        tex_filter d;
        d.idx = idx;
        d.minification = minification;
        d.magnification = magnification;
        d.mipmap = mipmap;
        d.aniso = aniso;
        m_current.write(CMD_TEX_FILTER, d);
    }

    void RoxRenderBuffered::removeTexture(int RoxTexture) { m_current.write(CMD_TEX_REMOVE, RoxTexture); }
    bool RoxRenderBuffered::isTextureFormatSupported(RoxTexture::COLOR_FORMAT format) { return m_tex_formats[format]; }

    int RoxRenderBuffered::createTarget(uint width, uint height, uint samples, const int* attachment_textures,
        const int* attachment_sides, uint attachment_count, int depth_texture)
    {
        target_create d;
        d.idx = newIdx();
        d.w = width;
        d.h = height;
        d.s = samples;
        d.count = attachment_count;
        for (uint i = 0; i < attachment_count; ++i)
        {
            d.at[i] = attachment_textures[i];
            d.as[i] = attachment_sides[i];
        }
        d.d = depth_texture;
        m_current.write(CMD_TARGET_CREATE, d);
        return d.idx;
    }

    void RoxRenderBuffered::removeTarget(int idx) { m_current.write(CMD_TARGET_REMOVE, idx); }
    void RoxRenderBuffered::invalidateCachedState() { m_current.write(CMD_INVALIDATE); }

    //----------------------------------------------------------------

    static const int invalid_idx = -1;

    //----------------------------------------------------------------

    int RoxRenderBuffered::newIdx()
    {
        if (!m_current.remap_free.empty())
        {
            const int idx = m_current.remap_free.back();
            m_current.remap_free.pop_back();
            return idx;
        }

        const int idx = (int)m_current.remap.size();
        m_current.remap.push_back(invalid_idx);
        m_current.update_remap = true;
        return idx;
    }

    void RoxRenderBuffered::commit()
    {
        m_current.buffer.swap(m_pending.buffer);

        if (m_pending.update_remap)
        {
            if (!m_pending.remap_free.empty())
            {
                m_current.remap_free.insert(m_current.remap_free.end(), m_pending.remap_free.begin(), m_pending.remap_free.end());
                m_pending.remap_free.clear();
            }

            memcpy(&m_current.remap[0], &m_pending.remap[0], m_pending.remap.size() * sizeof(m_pending.remap[0]));
            m_pending.update_remap = false;
        }

        if (m_current.update_remap)
        {
            m_pending.remap = m_current.remap;
            m_current.update_remap = false;
            m_pending.update_remap = true;
        }
    }

    void RoxRenderBuffered::push()
    {
        m_pending.buffer.swap(m_processing.buffer);

        bool pending_changed = false;

        if (m_processing.update_remap)
        {
            memcpy(&m_pending.remap[0], &m_processing.remap[0], m_processing.remap.size() * sizeof(m_processing.remap[0]));
            m_processing.update_remap = false;
            pending_changed = true;

            if (!m_processing.remap_free.empty())
            {
                m_pending.remap_free.swap(m_processing.remap_free);
                m_processing.remap_free.clear();
            }
        }

        if (m_pending.update_remap)
            m_processing.remap = m_pending.remap;

        m_pending.update_remap = pending_changed;
    }

    void RoxRenderBuffered::remapIdx(int& idx) const { if (idx >= 0) { idx = m_processing.remap[idx]; } }

    void RoxRenderBuffered::remapState(State& s) const
    {
        remapIdx(s.target);
        remapIdx(s.vertex_buffer);
        remapIdx(s.index_buffer);
        remapIdx(s.shader);
        remapIdx(s.uniform_buffer);
        for (int i = 0; i < s.max_layers; ++i)
            remapIdx(s.textures[i]);
    }

    //----------------------------------------------------------------

    void RoxRenderBuffered::execute()
    {
        m_processing.read_offset = 0;
        while (m_processing.read_offset < m_processing.buffer.size())
        {
            COMMAND_TYPE cmd = m_processing.getCmd();
            switch (cmd)
            {
            case CMD_UNIFORM:
            {
                uniform_data d = m_processing.getCmdData<uniform_data>();
                d.buf_idx = m_processing.remap[d.buf_idx];
                if (d.buf_idx < 0)
                    m_processing.getFbuf(d.count);
                else
                    m_backend.setUniform(d.buf_idx, d.idx, m_processing.getFbuf(d.count), d.count);
                break;
            }

            case CMD_DRAW:
            {
                State& s = m_processing.getCmdData<State>();
                remapState(s);
                m_backend.draw(s);
                break;
            }

            case CMD_CAMERA:
            {
                camera_data& d = m_processing.getCmdData<camera_data>();
                m_backend.setCamera(d.mv, d.p);
                break;
            }

            case CMD_CLEAR:
            {
                clear_data& d = m_processing.getCmdData<clear_data>();
                remapIdx(d.vp.target);
                m_backend.clear(d.vp, d.color, d.depth, d.stencil);
                break;
            }

            case CMD_RESOLVE:
            {
                int idx = m_processing.getCmdData<int>();
                remapIdx(idx);
                m_backend.resolveTarget(idx);
                break;
            }

            case CMD_APPLY:
            {
                State& s = m_processing.getCmdData<State>();
                remapState(s);
                m_backend.applyState(s);
                break;
            }

            case CMD_SHDR_CREATE:
            {
                const shader_create_data d = m_processing.getCmdData<shader_create_data>();
                const char* vs = (char*)m_processing.getCbuf(d.vs_size);
                const char* ps = (char*)m_processing.getCbuf(d.ps_size);
                m_processing.remap[d.idx] = m_backend.createShader(vs, ps);
                m_processing.update_remap = true;
                break;
            }

            case CMD_SHDR_REMOVE:
            {
                const int idx = m_processing.getCmdData<int>();
                const int ridx = m_processing.remap[idx];
                if (ridx < 0)
                    break;

                m_backend.removeShader(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap = true;
                break;
            }

            case CMD_UBUF_CREATE:
            {
                ubuf_create_data& d = m_processing.getCmdData<ubuf_create_data>();
                remapIdx(d.shader_idx);
                m_processing.remap[d.idx] = m_backend.createUniformBuffer(d.shader_idx);
                m_processing.update_remap = true;
                break;
            }

            case CMD_UBUF_REMOVE:
            {
                const int idx = m_processing.getCmdData<int>();
                const int ridx = m_processing.remap[idx];
                if (ridx < 0)
                    break;

                m_backend.removeUniformBuffer(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap = true;
                break;
            }

            case CMD_VBUF_CREATE:
            {
                const vbuf_create_data d = m_processing.getCmdData<vbuf_create_data>();
                const void* data = m_processing.getCbuf(d.count * d.stride);
                m_processing.remap[d.idx] = m_backend.createVertexBuffer(data, d.stride, d.count, d.usage);
                m_processing.update_remap = true;
                break;
            }

            case CMD_VBUF_LAYOUT:
            {
                vbuf_layout& d = m_processing.getCmdData<vbuf_layout>();
                remapIdx(d.idx);
                if (d.idx >= 0)
                    m_backend.setVertexLayout(d.idx, d.layout);
                break;
            }

            case CMD_VBUF_REMOVE:
            {
                const int idx = m_processing.getCmdData<int>();
                const int ridx = m_processing.remap[idx];
                if (ridx < 0)
                    break;

                m_backend.removeVertexBuffer(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap = true;
                break;
            }

            case CMD_IBUF_CREATE:
            {
                const ibuf_create_data d = m_processing.getCmdData<ibuf_create_data>();
                const void* data = m_processing.getCbuf(d.count * d.type);
                m_processing.remap[d.idx] = m_backend.createIndexBuffer(data, d.type, d.count, d.usage);
                m_processing.update_remap = true;
                break;
            }

            case CMD_IBUF_REMOVE:
            {
                const int idx = m_processing.getCmdData<int>();
                const int ridx = m_processing.remap[idx];
                if (ridx < 0)
                    break;

                m_backend.removeIndexBuffer(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap = true;
                break;
            }

            case CMD_TEX_CREATE:
            {
                tex_create_data d = m_processing.getCmdData<tex_create_data>();
                const void* data = d.size > 0 ? m_processing.getCbuf(d.size) : 0;
                m_processing.remap[d.idx] = m_backend.createTexture(data, d.width, d.height, d.format, d.mip_count);
                m_processing.update_remap = true;
                break;
            }

            case CMD_TEX_CUBE:
            {
                tex_create_data d = m_processing.getCmdData<tex_create_data>();
                const void* data[6] = { 0 };
                if (d.size > 0)
                {
                    for (int i = 0; i < 6; ++i)
                        data[i] = m_processing.getCbuf(d.size);
                }
                m_processing.remap[d.idx] = m_backend.createCubemap(data, d.width, d.format, d.mip_count);
                m_processing.update_remap = true;
                break;
            }

            case CMD_TEX_UPDATE:
            {
                tex_update d = m_processing.getCmdData<tex_update>();
                remapIdx(d.idx);
                const void* buf = m_processing.getCbuf(d.size);
                if (d.idx >= 0)
                    m_backend.updateTexture(d.idx, buf, d.x, d.y, d.width, d.height, d.mip);
                break;
            }

            case CMD_TEX_WRAP:
            {
                tex_wrap& d = m_processing.getCmdData<tex_wrap>();
                remapIdx(d.idx);
                if (d.idx >= 0)
                    m_backend.setTextureWrap(d.idx, d.s, d.t);
                break;
            }

            case CMD_TEX_FILTER:
            {
                tex_filter& d = m_processing.getCmdData<tex_filter>();
                remapIdx(d.idx);
                if (d.idx >= 0)
                    m_backend.setTextureFilter(d.idx, d.minification, d.magnification, d.mipmap, d.aniso);
                break;
            }

            case CMD_TEX_REMOVE:
            {
                const int idx = m_processing.getCmdData<int>();
                const int ridx = m_processing.remap[idx];
                if (ridx < 0)
                    break;

                m_backend.removeTexture(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap = true;
                break;
            }

            case CMD_TARGET_CREATE:
            {
                target_create& d = m_processing.getCmdData<target_create>();
                for (uint i = 0; i < d.count; ++i)
                    remapIdx(d.at[i]);
                remapIdx(d.d);
                m_processing.remap[d.idx] = m_backend.createTarget(d.w, d.h, d.s, d.at, d.as, d.count, d.d);
                m_processing.update_remap = true;
                break;
            }

            case CMD_TARGET_REMOVE:
            {
                const int idx = m_processing.getCmdData<int>();
                const int ridx = m_processing.remap[idx];
                if (ridx < 0)
                    break;

                m_backend.removeTarget(ridx);
                m_processing.remap_free.push_back(idx);
                m_processing.update_remap = true;
                break;
            }

            default:
                log() << "unsupported render command: " << cmd << "\n"; m_processing.buffer.clear(); return;
            }
        }

        m_processing.buffer.clear();
    }

}