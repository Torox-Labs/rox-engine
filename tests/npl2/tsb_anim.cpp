//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "tsb_anim.h"
#include "memory/tmp_buffer.h"
#include <string.h>

bool tsb_anim::load(nya_resources::resource_data *anim_res)
{
    if(!anim_res)
    {
        nya_resources::log()<<"Unable to load animation: invalid data\n";
        return false;
    }

    const size_t size=anim_res->get_size();
  
    nya_memory::tmp_buffer_scoped anim_data(size);
    anim_res->read_all(anim_data.get_data());
    anim_res->release();

    typedef unsigned int uint;

    struct tsb_header
    {
        const char magic[4];
        uint pad[3];
        uint bone_count;
        uint frame_count;
        uint first_loop_frame;
        uint special_count;
    };

    if(sizeof(tsb_header)>=size)
        return false;

    tsb_header *header=(tsb_header*)anim_data.get_data();
    if(strncmp(header->magic,"TSB0",4)!=0)
    {
        nya_resources::log()<<"Invalid TSB0 magic in animation file\n";
        //return false;
    }

    m_bones_count=header->bone_count;
    m_frames_count=header->frame_count;
    m_first_loop_frame=header->first_loop_frame;

    uint bones_size=header->bone_count*header->frame_count;
    m_data.resize(bones_size);
    if(!anim_data.copy_to(&m_data[0],bones_size*sizeof(nya_math::mat4),sizeof(tsb_header)))
    {
        nya_resources::log()<<"Unable to load tsb animation file\n";
        return false;
    }

    return true;
}

bool shared_anims_manager::fill_resource(const char *name,tsb_anim &res)
{
    if(!name)
    {
        nya_resources::log()<<"Unable to access model: invalid name\n";
        return false;
    }

    nya_resources::resource_data *data = nya_resources::get_resources_provider().access(name);

    return res.load(data);
}

bool shared_anims_manager::release_resource(tsb_anim &res)
{
    res.release();
    return true;
}

shared_anims_manager &get_shared_anims()
{
    static shared_anims_manager manager;
    return manager;
}
