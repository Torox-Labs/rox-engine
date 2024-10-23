// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "material.h"
#include "formats/text_parser.h"
#include "formats/string_convert.h"
#include "memory/invalid_object.h"
#include <list>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

namespace RoxScene
{

    const char* RoxMaterial::default_pass = "default";
    const char* RoxMaterialInternal::default_pass = RoxMaterial::default_pass;

    namespace
    {
        bool enable_highlight_missing_texture = true;

        std::vector<std::pair<std::string, texture_proxy>> global_textures_replace;

        bool isShaderSamplerCube(const shader& sh, unsigned int layer)
        {
            if (!sh.internal().get_shared_data().is_valid())
                return false;

            const RoxRender::shader& rsh = sh.internal().get_shared_data()->shdr;
            for (int i = 0; i < rsh.get_uniforms_count(); ++i)
            {
                if (rsh.get_uniform_type(i) != RoxRender::shader::uniform_sampler_cube)
                    continue;

                if (rsh.get_sampler_layer(rsh.get_uniform_name(i)) == static_cast<int>(layer))
                    return true;
            }

            return false;
        }

        unsigned long getTime()
        {
#ifdef _WIN32
            static LARGE_INTEGER freq;
            static bool initialised = false;
            if (!initialised)
            {
                QueryPerformanceFrequency(&freq);
                initialised = true;
            }

            LARGE_INTEGER time;
            QueryPerformanceCounter(&time);

            return static_cast<unsigned long>((time.QuadPart * 1000) / freq.QuadPart);
#else
            timeval tim;
            gettimeofday(&tim, nullptr);
            unsigned long sec = static_cast<unsigned long>(tim.tv_sec);
            return (sec * 1000 + (tim.tv_usec / 1000));
#endif
        }

        texture missingTexture(bool cube)
        {
            if (!enable_highlight_missing_texture)
            {
                static texture invalid;
                return invalid;
            }

            static texture missing_red;
            static texture missing_white;

            static texture missing_cube_red;
            static texture missing_cube_white;

            static bool initialised = false;
            if (!initialised)
            {
                const unsigned char red_data[4] = { 255, 0, 0, 255 };
                const unsigned char white_data[4] = { 255, 255, 255, 255 };

                shared_texture red_res;
                red_res.tex.build_texture(red_data, 1, 1, RoxRender::texture::color_rgba);
                missing_red.create(red_res);

                shared_texture white_res;
                white_res.tex.build_texture(white_data, 1, 1, RoxRender::texture::color_rgba);
                missing_white.create(white_res);

                const void* cube_red_data[6] = { red_data, red_data, red_data, red_data, red_data, red_data };
                const void* cube_white_data[6] = { white_data, white_data, white_data, white_data, white_data, white_data };

                shared_texture cube_red_res;
                cube_red_res.tex.build_cubemap(cube_red_data, 1, 1, RoxRender::texture::color_rgba);
                missing_cube_red.create(cube_red_res);

                shared_texture cube_white_res;
                cube_white_res.tex.build_cubemap(cube_white_data, 1, 1, RoxRender::texture::color_rgba);
                missing_cube_white.create(cube_white_res);

                initialised = true;
            }

            if ((getTime() / 200) % 2 > 0)
                return cube ? missing_cube_white : missing_white;

            return cube ? missing_cube_red : missing_red;
        }
    }

    void RoxMaterial::ParamArray::set(int idx, float f0, float f1, float f2, float f3)
    {
        if (idx >= 0 && idx < static_cast<int>(m_params.size()))
            m_params[idx].set(f0, f1, f2, f3);
    }

    void RoxMaterial::ParamArray::set(int idx, const RoxMath::vec3& v, float w)
    {
        if (idx >= 0 && idx < static_cast<int>(m_params.size()))
            m_params[idx].set(v, w);
    }

    const RoxMaterial::Param& RoxMaterial::ParamArray::get(int idx) const
    {
        if (idx < 0 || idx >= static_cast<int>(m_params.size()))
            return RoxMemory::invalid_object<Param>();

        return m_params[idx];
    }

    RoxMaterial::Param& RoxMaterial::ParamArray::get(int idx)
    {
        if (idx < 0 || idx >= static_cast<int>(m_params.size()))
            return RoxMemory::invalid_object<Param>();

        return m_params[idx];
    }

    void RoxMaterialInternal::ParamHolder::applyToShader(const shader& shader, int uniform_idx) const
    {
        if (p.is_valid())
            shader.internal().set_uniform_value(uniform_idx, p->x, p->y, p->z, p->w);
        else if (a.is_valid() && a->get_count() > 0)
            shader.internal().set_uniform4_array(uniform_idx, a->getBuf(), a->get_count());
        else
            shader.internal().set_uniform_value(uniform_idx, 0, 0, 0, 0);
    }

    void RoxMaterialInternal::skeletonChanged(const RoxRender::Skeleton* skeleton) const
    {
        for (int i = 0; i < static_cast<int>(m_passes.size()); ++i)
            m_passes[i].m_shader.internal().skeletonChanged(skeleton);
    }

    bool RoxMaterial::loadText(shared_material& res, resource_data& data, const char* name)
    {
        RoxFormats::text_parser parser;
        parser.load_from_data(reinterpret_cast<const char*>(data.get_data()), data.get_size());
        for (int section_idx = 0; section_idx < parser.get_sections_count(); ++section_idx)
        {
            const char* section_type = parser.get_section_type(section_idx);
            if (strcmp(section_type, "@pass") == 0)
            {
                int pass_idx = res.addPass(parser.get_section_name(section_idx));
                Pass& p = res.getPass(pass_idx);

                for (int subsection_idx = 0; subsection_idx < parser.get_subsections_count(section_idx); ++subsection_idx)
                {
                    const char* subsection_type = parser.get_subsection_type(section_idx, subsection_idx);
                    const char* subsection_value = parser.get_subsection_value(section_idx, subsection_idx);
                    if (!subsection_type || !subsection_value)
                        continue;

                    if (strcmp(subsection_type, "shader") == 0)
                    {
                        if (!p.m_shader.load(subsection_value))
                        {
                            log() << "can't load shader when loading material '" << name << "'\n";
                            return false;
                        }
                        continue;
                    }

                    if (strcmp(subsection_type, "blend") == 0)
                    {
                        RoxRender::State& s = p.getState();
                        s.blend = RoxFormats::blend_mode_from_string(subsection_value, s.blend_src, s.blend_dst);
                        continue;
                    }

                    if (strcmp(subsection_type, "zwrite") == 0)
                    {
                        p.getState().zwrite = parser.get_subsection_value_bool(section_idx, subsection_idx);
                        continue;
                    }

                    if (strcmp(subsection_type, "cull") == 0)
                    {
                        RoxRender::State& s = p.getState();
                        s.cull_face = RoxFormats::cull_face_from_string(subsection_value, s.cull_order);
                        continue;
                    }

                    if (strcmp(subsection_type, "depth_test") == 0)
                    {
                        p.getState().depth_test = parser.get_subsection_value_bool(section_idx, subsection_idx);
                        continue;
                    }

                    p.setPassParam(subsection_type, Param(RoxFormats::vec4_from_string(subsection_value)));
                }
            }
            else if (strcmp(section_type, "@texture") == 0)
            {
                texture_proxy tex;
                tex.create();
                if (tex->load(parser.get_section_value(section_idx)))
                {
                    const int texture_idx = res.getTextureIdx(parser.get_section_name(section_idx));
                    if (texture_idx < 0)
                    {
                        RoxMaterialInternal::MaterialTexture mat;
                        mat.semantics = parser.get_section_name(section_idx);
                        mat.proxy = tex;
                        res.m_textures.push_back(mat);
                    }
                    else
                        res.m_textures[texture_idx].proxy = tex;
                }
                else
                    log() << "can't load texture when loading material '" << name << "'\n";
            }
            else if (strcmp(section_type, "@param") == 0)
            {
                RoxMaterialInternal::ParamHolder ph;
                ph.name = parser.get_section_name(section_idx);
                ph.p = ParamProxy(RoxMaterialInternal::Param(parser.get_section_value_vec4(section_idx)));

                const int param_idx = res.getParamIdx(parser.get_section_name(section_idx));
                if (param_idx >= 0)
                    res.m_params[param_idx] = ph;
                else
                    res.m_params.push_back(ph);
            }
            else
                log() << "unknown section when loading material '" << name << "'\n";
        }

        res.m_should_rebuild_passes_maps = true;
        return true;
    }

    void RoxMaterialInternal::set(const char* pass_name) const
    {
        if (!pass_name)
            return;

        if (m_last_set_pass_idx >= 0)
            unset();

        m_last_set_pass_idx = getPassIdx(pass_name);
        if (m_last_set_pass_idx < 0)
            return;

        updatePassesMaps();
        const RoxPass& p = m_passes[m_last_set_pass_idx];

        p.m_shader.internal().set();
        for (int uniform_idx = 0; uniform_idx < static_cast<int>(p.m_uniforms_idxs_map.size()); ++uniform_idx)
            m_params[p.m_uniforms_idxs_map[uniform_idx]].applyToShader(p.m_shader, uniform_idx);

        for (int i = 0; i < static_cast<int>(p.m_pass_params.size()); ++i)
        {
            const RoxPass::PassParam& pp = p.m_pass_params[i];
            p.m_shader.internal().set_uniform_value(pp.uniform_idx, pp.p.x, pp.p.y, pp.p.z, pp.p.w);
        }

        RoxRender::set_state(p.m_render_state);

        for (int slot_idx = 0; slot_idx < static_cast<int>(p.m_textures_slots_map.size()); ++slot_idx)
        {
            int texture_idx = p.m_textures_slots_map[slot_idx];
            if (texture_idx < 0)
                continue;

            if (m_textures[texture_idx].proxy.is_valid())
            {
                if (!m_textures[texture_idx].proxy->internal().set(slot_idx))
                {
                    rox_log::warning() << "invalid texture for semantics '" << p.m_shader.internal().get_texture_semantics(slot_idx) << "' in material '" << m_name << "'\n";
                    missingTexture(isShaderSamplerCube(p.m_shader, slot_idx)).internal().set(slot_idx);
                }
            }
            else
            {
                rox_log::warning() << "invalid texture proxy for semantics '" << p.m_shader.internal().get_texture_semantics(slot_idx) << "' in material '" << m_name << "'\n";
                missingTexture(isShaderSamplerCube(p.m_shader, slot_idx)).internal().set(slot_idx);
            }
        }

        for (size_t i = 0; i < global_textures_replace.size(); ++i)
        {
            const int slot = p.m_shader.internal().get_texture_slot(global_textures_replace[i].first.c_str());
            if (slot < 0)
                continue;

            if (!global_textures_replace[i].second.is_valid())
                continue;

            global_textures_replace[i].second->internal().set(slot);
        }
    }

    void RoxMaterialInternal::unset() const
    {
        if (m_last_set_pass_idx < 0)
            return;

        const RoxPass& p = m_passes[m_last_set_pass_idx];
        p.m_shader.internal().unset();

        if (p.m_render_state.blend)
            RoxRender::blend::disable();

        if (!p.m_render_state.zwrite)
            RoxRender::zwrite::enable();

        if (!p.m_render_state.color_write)
            RoxRender::color_write::enable();

        for (int slot_idx = 0; slot_idx < static_cast<int>(p.m_textures_slots_map.size()); ++slot_idx)
        {
            const int texture_idx = static_cast<int>(p.m_textures_slots_map[slot_idx]);
            if (texture_idx >= 0 && texture_idx < static_cast<int>(m_textures.size()) && m_textures[texture_idx].proxy.is_valid())
                m_textures[texture_idx].proxy->internal().unset();
        }

        m_last_set_pass_idx = -1;
    }

    int RoxMaterialInternal::getParamIdx(const char* name) const
    {
        if (!name)
            return -1;

        updatePassesMaps();
        for (int i = 0; i < static_cast<int>(m_params.size()); ++i)
        {
            if (m_params[i].name == name)
                return i;
        }

        return -1;
    }

    int RoxMaterialInternal::getTextureIdx(const char* semantics) const
    {
        if (!semantics)
            return -1;

        for (int i = 0; i < static_cast<int>(m_textures.size()); ++i)
        {
            if (m_textures[i].semantics == semantics)
                return i;
        }

        return -1;
    }

    RoxMaterialInternal::RoxPass& RoxMaterialInternal::RoxPass::operator=(const RoxPass& p)
    {
        m_name = p.m_name;
        m_render_state = p.m_render_state;
        m_shader = p.m_shader;
        m_pass_params = p.m_pass_params;
        m_shader_changed = true;
        m_uniforms_idxs_map.clear();
        m_textures_slots_map.clear();
        return *this;
    }

    void RoxMaterialInternal::RoxPass::setShader(const shader& shader)
    {
        m_shader = shader;
        m_shader_changed = true;
        m_uniforms_idxs_map.clear();
        m_textures_slots_map.clear();
        updatePassParams();
    }

    void RoxMaterialInternal::RoxPass::setPassParam(const char* name, const Param& value)
    {
        if (!name)
            return;

        for (int i = 0; i < static_cast<int>(m_pass_params.size()); ++i)
        {
            if (m_pass_params[i].name == name)
            {
                m_pass_params[i].p = value;
                updatePassParams();
                return;
            }
        }

        m_pass_params.emplace_back();
        m_pass_params.back().name = name;
        m_pass_params.back().p = value;

        updatePassParams();
    }

    void RoxMaterialInternal::RoxPass::updatePassParams()
    {
        for (int i = 0; i < static_cast<int>(m_pass_params.size()); ++i)
        {
            PassParam& pp = m_pass_params[i];
            pp.uniform_idx = -1;

            for (int uniform_idx = 0; uniform_idx < m_shader.internal().get_uniforms_count(); ++uniform_idx)
            {
                const std::string& name = m_shader.internal().get_uniform(uniform_idx).name;
                if (pp.name == name)
                {
                    pp.uniform_idx = uniform_idx;
                    break;
                }
            }
        }
    }

    void RoxMaterialInternal::RoxPass::updateMaps(const RoxMaterialInternal& m) const
    {
        m_uniforms_idxs_map.resize(m_shader.internal().get_uniforms_count());
        std::fill(m_uniforms_idxs_map.begin(), m_uniforms_idxs_map.end(), 0); // Params should exist if idxs_map was rebuilt properly
        for (int uniform_idx = 0; uniform_idx < m_shader.internal().get_uniforms_count(); ++uniform_idx)
        {
            const std::string name = m_shader.internal().get_uniform(uniform_idx).name;
            // Don't use m.getParamIdx(name.c_str()) as it calls rebuild_passes_map
            int param_idx = -1;
            for (int i = 0; i < static_cast<int>(m.m_params.size()); ++i)
            {
                if (m.m_params[i].name == name)
                {
                    param_idx = i;
                    break;
                }
            }

            if (param_idx >= 0)
                m_uniforms_idxs_map[uniform_idx] = param_idx;
        }

        m_textures_slots_map.resize(m_shader.internal().get_texture_slots_count());
        std::fill(m_textures_slots_map.begin(), m_textures_slots_map.end(), -1);
        for (int slot_idx = 0; slot_idx < m_shader.internal().get_texture_slots_count(); ++slot_idx)
        {
            const char* semantics = m_shader.internal().get_texture_semantics(slot_idx);
            if (!semantics || !semantics[0])
                continue;

            const int texture_idx = m.getTextureIdx(semantics);
            if (texture_idx >= 0)
                m_textures_slots_map[slot_idx] = texture_idx;
        }
    }

    int RoxMaterialInternal::addPass(const char* pass_name)
    {
        if (!pass_name)
            return -1;

        updatePassesMaps();

        for (auto iter = m_passes.begin(); iter != m_passes.end(); ++iter)
        {
            if (iter->m_name == pass_name)
                return static_cast<int>(std::distance(m_passes.begin(), iter));
        }

        m_passes.emplace_back();
        m_passes.back().m_name.assign(pass_name);
        return static_cast<int>(m_passes.size()) - 1;
    }

    int RoxMaterialInternal::getPassIdx(const char* pass_name) const
    {
        if (!pass_name)
            return -1;

        for (int i = 0; i < static_cast<int>(m_passes.size()); ++i)
            if (m_passes[i].m_name == pass_name)
                return i;

        return -1;
    }

    RoxMaterialInternal::RoxPass& RoxMaterialInternal::getPass(int idx)
    {
        if (idx < 0 || idx >= static_cast<int>(m_passes.size()))
            return RoxMemory::invalid_object<RoxPass>();

        return m_passes[idx];
    }

    const RoxMaterialInternal::RoxPass& RoxMaterialInternal::getPass(int idx) const
    {
        if (idx < 0 || idx >= static_cast<int>(m_passes.size()))
            return RoxMemory::invalid_object<RoxPass>();

        return m_passes[idx];
    }

    void RoxMaterialInternal::removePass(const char* pass_name)
    {
        const int idx = getPassIdx(pass_name);
        if (idx < 0)
            return;

        m_passes.erase(m_passes.begin() + idx);
        m_should_rebuild_passes_maps = true;
    }

    void RoxMaterialInternal::updatePassesMaps() const
    {
        if (!m_should_rebuild_passes_maps)
        {
            for (auto it = m_passes.begin(); it != m_passes.end(); ++it)
            {
                if (it->m_shader_changed)
                {
                    m_should_rebuild_passes_maps = true;
                    break;
                }
            }
        }

        if (!m_should_rebuild_passes_maps)
            return;

        if (m_passes.empty())
            return;

        // Step 1: Build params array
        std::list<std::pair<std::string, RoxMath::vec4>> parameters_to_add;
        std::list<std::pair<std::string, int>> param_arrays_to_add;
        for (int pass_idx = 0; pass_idx < static_cast<int>(m_passes.size()); ++pass_idx)
        {
            const shader& sh = m_passes[pass_idx].m_shader;
            for (int uniform_idx = 0; uniform_idx < sh.internal().get_uniforms_count(); ++uniform_idx)
            {
                const std::string name = sh.internal().get_uniform(uniform_idx).name;
                // Don't use getParamIdx(name.c_str()) as it calls rebuild_passes_map
                int param_idx = -1;
                for (int i = 0; i < static_cast<int>(m_params.size()); ++i)
                {
                    if (m_params[i].name == name)
                    {
                        param_idx = i;
                        break;
                    }
                }

                if (param_idx >= 0)
                {
                    continue;
                }

                const int count = sh.internal().get_uniform_array_size(uniform_idx);
                if (count > 1)
                    param_arrays_to_add.emplace_back(name, count);
                else
                    parameters_to_add.emplace_back(name, sh.internal().get_uniform(uniform_idx).default_value);
            }
        }

        // Add missing parameters
        for (auto& iter : parameters_to_add)
        {
            m_params.emplace_back();
            m_params.back().name = iter.first;
            m_params.back().p.create(iter.second);
        }

        for (auto& iter : param_arrays_to_add)
        {

