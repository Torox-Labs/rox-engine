//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "shaders_cache_provider.h"
#include "resources/resources.h"

#include <string.h>
#include <stdio.h>

//ToDo: hash collisions

namespace nya_system
{

bool compiled_shaders_provider::get(const char *text,nya_render::compiled_shader &shader)
{
    shader=nya_render::compiled_shader();

    if(!text)
        return false;

    nya_resources::resource_data *data=
    nya_resources::get_resources_provider().access((m_load_path+crc(text)+".nsc").c_str());
    if(!data)
        return false;

    shader=nya_render::compiled_shader(data->get_size());
    data->read_all(shader.get_data());
    data->release();

    return true;
}

bool compiled_shaders_provider::set(const char *text,const nya_render::compiled_shader &shader)
{
    if(!text)
        return false;

    const void *data=shader.get_data();
    if(!data)
        return false;

    FILE *f=fopen((m_save_path+crc(text)+".nsc").c_str(),"wb");
    if(!f)
        return false;

    fwrite(data,shader.get_size(),1,f);
    fclose(f);

    return true;
}

std::string compiled_shaders_provider::crc(const char *text)
{
    if(!text)
        return "";

    static unsigned int crc_table[256];
    static bool initialised=false;
    if(!initialised)
    {
        for(int i=0;i<256;i++)
        {
            unsigned int crc=i;
            for(int j=0;j<8;j++)
                crc=crc&1?(crc>>1)^0xEDB88320UL:crc>>1;

            crc_table[i]=crc;
        };

        initialised=true;
    }

    unsigned int crc=0xFFFFFFFFUL;
    size_t len=strlen(text);
    unsigned char *buf=(unsigned char*)text;
    while(len--)
        crc=crc_table[(crc^ *buf++)&0xFF]^(crc>>8);

    char tmp[256];
    sprintf(tmp,"%ud",crc);

    return std::string(tmp);
}

}
