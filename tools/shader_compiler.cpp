//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include <stdio.h>
#include <string.h>
#include "log/log.h"
#include "render/shader_code_parser.h"
#include "formats/text_parser.h"
#include "system/shaders_cache_provider.h"
#include "resources/file_resources_provider.h"
#include <D3Dcompiler.h>
#include <io.h>
#include <fcntl.h>

#pragma comment(lib, "D3DCompiler.lib")
#pragma warning(disable: 4996)

const char *help="Usage: shader_compiler %%mode%%\n"
                 "accepts shader's code from stdin\n"
                 "stderr output begins with Error: if something goes wrong\n"
                 "outputs compiled shader/text assembly to stdout\n"
                 "modes:\n"
                 "hlsl - compiles hlsl shader\n"
                 "hlsl2asm - compiles hlsl shader and returns text assembly\n"
                 "glsl2hlsl - converts glsl shader to hlsl\n"
                 "or use: shader_compiler gencache %%src_dir%% %%dst_dir%%"
                 "\n";

ID3D10Blob *compile_hlsl(const char *code)
{
    const bool is_ps=strstr(code,"SV_TARGET")!=0;
    const char *profile=is_ps?"ps_4_0_level_9_3":"vs_4_0_level_9_3";
    ID3D10Blob *compiled=0, *error=0;
    D3DCompile(code,strlen(code),0,0,0,"main",profile,0,0,&compiled,&error);
    if(error)
    {
        fprintf(stderr,"Error: can`t compile shader with profile %s\n",profile);
        std::string error_text((const char *)error->GetBufferPointer(),error->GetBufferSize());
        fprintf(stderr,"%s\n",error_text.c_str());
        error->Release();
        return 0;
    }

    if(!compiled)
        fprintf(stderr,"Error: compile error\n");

    return compiled;
}

bool compile_hlsl_code(const char *code,bool text_asm)
{
    if(!code)
        return false;

    ID3D10Blob *compiled=compile_hlsl(code);
    if(!compiled)
        return false;

    if(text_asm)
    {
        ID3D10Blob *asm_blob;
        D3DDisassemble(compiled->GetBufferPointer(),compiled->GetBufferSize(),
                       D3D_DISASM_ENABLE_INSTRUCTION_NUMBERING,"",&asm_blob);
        if(!asm_blob)
            return false;

        fwrite(asm_blob->GetBufferPointer(),1,asm_blob->GetBufferSize(),stdout);
        asm_blob->Release();
    }
    else
        fwrite(compiled->GetBufferPointer(),1,compiled->GetBufferSize(),stdout);

    compiled->Release();
    return true;
}

bool load_nya_shader(const char* name,std::string &code_vs,std::string &code_ps)
{
    nya_resources::resource_data *rdata=nya_resources::get_resources_provider().access(name);
    if(!rdata)
        return false;

    std::string shader_text;
    shader_text.resize(rdata->get_size());
    rdata->read_all(&shader_text[0]);
    rdata->release();

    nya_formats::text_parser parser;
    parser.load_from_data((const char *)shader_text.c_str());
    for(int section_idx=0;section_idx<parser.get_sections_count();++section_idx)
    {
        const char *section_type=parser.get_section_type(section_idx);
        if(strcmp(section_type,"@include")==0)
        {
            const char *file=parser.get_section_name(section_idx);
            if(!file)
            {
                nya_log::log()<<"unable to load shader include in shader "<<name<<": invalid filename\n";
                return false;
            }

            std::string path(name);
            size_t p=path.rfind("/");
            if(p==std::string::npos)
                p=path.rfind("\\");

            if(p==std::string::npos)
                path.clear();
            else
                path.resize(p+1);

            path.append(file);

            load_nya_shader(path.c_str(),code_vs,code_ps);
        }
        else if(strcmp(section_type,"@all")==0)
        {
            const char *text=parser.get_section_value(section_idx);
            if(text)
            {
                code_vs.append(text);
                code_ps.append(text);
            }
        }
        else if(strcmp(section_type,"@vertex")==0)
        {
            const char *text=parser.get_section_value(section_idx);
            if(text)
                code_vs.append(text);
        }
        else if(strcmp(section_type,"@fragment")==0)
        {
            const char *text=parser.get_section_value(section_idx);
            if(text)
                code_ps.append(text);
        }
    }

    return true;
}

bool generate_cache( const char* dir_from,  const char* dir_to, bool recursive )
{
    if(!dir_from || !dir_to)
        return false;

    nya_system::compiled_shaders_provider csp;
    csp.set_save_path((std::string(dir_to)+"\\").c_str());

    nya_resources::file_resources_provider fp;
    nya_resources::set_resources_provider(&fp);
    fp.set_folder(dir_from);
    std::string shader_text;
    for(int i=0;i<fp.get_resources_count();++i)
    {
        const char *name=fp.get_resource_name(i);
        if(!nya_resources::check_extension(name,".nsh"))
            continue;

        std::string code[2];
        load_nya_shader(name,code[0],code[1]);

        const char *st[2]={"vs","ps"};
        for(int j=0;j<2;++j)
        {
            nya_render::shader_code_parser parser(code[j].c_str(),"_nya_",j==0?"_nya_flip_y_":"");

            if(!parser.convert_to_hlsl())
            {
                fprintf(stderr,"Error: cannot convert to hlsl %s shader from %s\n",st[j],name);
                continue;
            }

            ID3D10Blob *blob=compile_hlsl(parser.get_code());
            if(!blob)
            {
                fprintf(stderr,"Error: cannot compile %s shader from %s\n",st[j],name);
                continue;
            }

            nya_render::compiled_shader cs(blob->GetBufferSize());
            memcpy(cs.get_data(),blob->GetBufferPointer(),blob->GetBufferSize());
            blob->Release();
            csp.set(code[j].c_str(),cs);
        }
    }

    return true;
}

int main(int argc, char* argv[])
{
    if(argc<2)
    {
        fprintf(stderr,"Error: no arguments\n");
        printf("%s",help);
        return 0;
    }

    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);

    nya_log::set_log(&nya_log::no_log());
    
    if(strcmp(argv[1],"gencache")==0)
    {
        if(argc!=4)
        {
            fprintf(stderr,"Error: src and dst dir not specified\n");
            printf("Usage: shader_compiler gencache %%src_dir%% %%dst_dir%%\n");
            return -1;
        }

        return generate_cache(argv[2],argv[3],true)?0:-1;
    }

    std::string shader_code;
    char buf[512];
    for(size_t size=fread(buf,1,sizeof(buf),stdin);size>0;size=fread(buf,1,sizeof(buf),stdin))
        shader_code.append(buf,size);

    if(shader_code.empty())
    {
        fprintf(stderr,"Error: empty stdin\n");
        printf("%s",help);
        return 0;
    }

    if(strcmp(argv[1],"hlsl")==0)
        return compile_hlsl_code(shader_code.c_str(),false)?0:-1;

    if(strcmp(argv[1],"hlsl2asm")==0)
        return compile_hlsl_code(shader_code.c_str(),true)?0:-1;

    if(strcmp(argv[1],"glsl2hlsl")==0)
    {
        nya_render::shader_code_parser parser(shader_code.c_str(),"_nya_","_nya_flip_y_");
        if(!parser.convert_to_hlsl())
        {
            fprintf(stderr,"Error: cannot convert to hlsl\n\n\n");
            return -1;
        }

        printf("%s",parser.get_code());
        return 0;
    }

    fprintf(stderr,"Error: invalid compile mode: %s\n\n\n",argv[1]);
	return -1;
}
