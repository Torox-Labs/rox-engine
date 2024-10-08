//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//requires: libdevil http://openil.sourceforge.net/

#include "texture_il.h"
#include "scene/scene.h"

#include "IL/il.h"

namespace nya_scene
{
    
void init_il()
{
    static bool once=true;
    if(!once)
        return;

    once=false;
    ilInit();
    ilEnable(IL_ORIGIN_SET);
    ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
}

bool load_texture_il(nya_scene::shared_texture &res,nya_scene::resource_data &data,const char* name)
{
    init_il();

    const ILuint img=ilGenImage();
    ilBindImage(img);

    if(!ilLoadL(IL_TYPE_UNKNOWN,data.get_data(),ILuint(data.get_size())))
    {
        ilDeleteImage(img);
        return false;
    }

    const ILint width=ilGetInteger(IL_IMAGE_WIDTH);
    const ILint height=ilGetInteger(IL_IMAGE_HEIGHT);
    ILint bpp=ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
    ILint img_format=ilGetInteger(IL_IMAGE_FORMAT);

    if(img_format==IL_COLOR_INDEX)
    {
        bpp=ilGetInteger(IL_PALETTE_BPP);
        if(bpp==4)
            img_format=IL_RGBA;
        else if(bpp==3)
            img_format=IL_RGB;
        else
        {
            nya_log::log()<<"load texture il error: incorrect pallete format: "<<img_format<<" bpp "<<bpp<<"\n";
            ilDeleteImage(img);
            return false;
        }
        ilConvertImage(img_format,IL_UNSIGNED_BYTE);
    }

    nya_render::texture::color_format format;
    switch(img_format)
    {
    case IL_LUMINANCE: format=nya_render::texture::greyscale; break;

    case IL_RGB: format=nya_render::texture::color_rgb;
            if(bpp>3)
                ilConvertImage(IL_RGB,IL_UNSIGNED_BYTE);
            break;

    case IL_BGR: format=nya_render::texture::color_rgb;
            ilConvertImage(IL_RGB,IL_UNSIGNED_BYTE);
            break;

    case IL_RGBA: format=nya_render::texture::color_rgba;
            if(bpp>4)
                ilConvertImage(IL_RGBA,IL_UNSIGNED_BYTE);
            break;

    case IL_BGRA: format=nya_render::texture::color_bgra;
            if(bpp>4)
                ilConvertImage(IL_BGRA,IL_UNSIGNED_BYTE);
            break;

    default:
        nya_log::log()<<"load texture il error: incorrect format: "<<img_format<<" bpp "<<bpp<<"\n";
        ilDeleteImage(img);
        return false;
    }
    res.tex.build_texture(ilGetData(),width,height,format);
    ilDeleteImage(img);
    texture::read_meta(res,data);
    return true;
}

bool save_texture_il(const texture &tex, const char *name)
{
    if(!name)
        return false;

    const unsigned int width=tex.get_width(),height=tex.get_height();
    if(!width || !height)
        return false;

    ILubyte bpp;
    ILenum format, type=IL_UNSIGNED_BYTE;

    switch(tex.get_format())
    {
        case nya_render::texture::greyscale:  bpp=1; format=IL_LUMINANCE; break;
        case nya_render::texture::color_rgb:  bpp=3; format=IL_RGB; break;
        case nya_render::texture::color_rgba: bpp=4; format=IL_RGBA; break;
        case nya_render::texture::color_bgra: bpp=4; format=IL_BGRA; break;
        default: return false;
    }

    nya_memory::tmp_buffer_ref data=tex.get_data();
    if(!data.get_data())
        return false;

    init_il();

    const ILuint img=ilGenImage();
    ilBindImage(img);
    ilTexImage(width,height,0,bpp,format,type,data.get_data());
    data.free();

    ilEnable(IL_FILE_OVERWRITE);
    const bool result=ilSaveImage(name);
    ilDeleteImage(img);
    return result;
}

}
