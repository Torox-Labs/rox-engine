//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxShadersCacheprovider.h"
#include "RoxResources/RoxResources.h"

#include <string.h>
#include <stdio.h>

//ToDo: hash collisions

namespace RoxSystem
{

    bool RoxCompiledShadersProvider::get(const char* text, RoxRender::compiled_shader& shader)
 {
    //shader = RoxRender::CompiledShader();
    shader = RoxRender::compiled_shader();

    if(!text)
        return false;

    RoxResources::resource_data*data=
    RoxResources::get_resources_provider().access((m_load_path+crc(text)+".nsc").c_str());
    if(!data)
        return false;

    shader=RoxRender::compiled_shader(data->get_size());
    data->read_all(shader.get_data());
    data->release();

    return true;
}

bool RoxCompiledShadersProvider::set(const char *text,const RoxRender::compiled_shader &shader)
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

std::string RoxCompiledShadersProvider::crc(const char *text)
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
    printf(tmp,"%ud",crc);

    return std::string(tmp);
}

}
