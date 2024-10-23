//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "particles.h"

namespace nya_scene
{

struct shared_particles_group
{
    std::vector<nya_scene::particles> particles;
    std::vector<std::pair<std::string,nya_math::vec4> > params;

    bool release()
    {
        particles.clear();
        return true;
    }
};

class particles_group: public scene_shared<shared_particles_group>
{
public:
    bool load(const char *name);
    void unload();

public:
    void reset_time();
    void update(unsigned int dt);
    void draw(const char *pass_name=material::default_pass) const;

public:
    void set_pos(const nya_math::vec3 &pos);
    void set_rot(const nya_math::quat &rot);
    void set_rot(nya_math::angle_deg yaw,nya_math::angle_deg pitch,nya_math::angle_deg roll);
    void set_scale(float s);
    void set_scale(const nya_math::vec3 &s);

public:
    const nya_math::vec3 &get_pos() const { return m_transform.get_pos(); }
    const nya_math::quat &get_rot() const { return m_transform.get_rot(); }
    const nya_math::vec3 &get_scale() const { return m_transform.get_scale(); }

public:
    int get_count() const;
    const particles &get(int idx) const;

public:
    particles_group() {}
    particles_group(const char *name) { load(name); }

public:
    static bool load_text(shared_particles_group &res,resource_data &data,const char* name);

private:
    std::vector<particles> m_particles;
    transform m_transform;
};

}
