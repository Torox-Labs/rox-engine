//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxShadersCacheprovider.h"
#include "RoxResources/RoxResources.h"

#include <string.h>
#include <stdio.h>

//ToDo: hash collisions

namespace RoxSystem
{

bool RoxCompiledShadersProvider::get(const char *text, RoxRender::CompiledShader &shader)
{
    shader=RoxRender::CompiledShader();

    if(!text)
        return false;

    RoxResources::RoxResourceData *data=
    RoxResources::getResourcesProvider().access((m_load_path+crc(text)+".nsc").c_str());
    if(!data)
        return false;

    shader=RoxRender::CompiledShader(data->getSize());
    data->readAll(shader.getData());
    data->release();

    return true;
}

bool RoxCompiledShadersProvider::set(const char *text,const RoxRender::CompiledShader &shader)
{
    if(!text)
        return false;

    const void *data=shader.getData();
    if(!data)
        return false;

    FILE *f=fopen((m_save_path+crc(text)+".nsc").c_str(),"wb");
    if(!f)
        return false;

    fwrite(data,shader.getSize(),1,f);
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
