//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include "shared_resources.h"
#include "render/animation.h"
#include "memory/optional.h"

namespace nya_scene
{

struct shared_animation
{
    nya_render::animation anim;

    bool release()
    {
        anim.release();
        return true;
    }
};

class animation: public scene_shared<shared_animation>
{
    friend class mesh_internal;

public:
    bool load(const char *name);
    void unload();

public:
    void create(const shared_animation &res);

public:
    void set_range(unsigned int from,unsigned int to);
    void set_weight(float weight) { m_weight=weight; }
    void set_speed(float speed) { m_speed=speed; }
    void set_loop(bool looped) { m_looped=looped; }

    unsigned int get_duration() const;
    float get_weight() const { return m_weight; }
    float get_speed() const { return m_speed; }
    bool get_loop() const { return m_looped; }

public:
    void mask_all(bool enabled);
    void add_mask(const char *name,bool enabled);

private:
    void update_version();

public:
    animation(): m_looped(true),m_range_from(0),m_range_to(0),m_speed(1.0f),m_weight(1.0f),m_version(0) {}
    animation(const char *name) { *this=animation(); load(name); }

public:
    static bool load_nan(shared_animation &res,resource_data &data,const char* name);

private:
    bool m_looped;
    unsigned int m_range_from;
    unsigned int m_range_to;
    float m_speed;
    float m_weight;

    unsigned int m_version;

    struct mask_data { std::map<std::string,bool> data; };

    nya_memory::optional<mask_data> m_mask;
};

}
