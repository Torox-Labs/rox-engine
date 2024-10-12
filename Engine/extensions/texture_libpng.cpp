//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//requires: libpng http://www.libpng.org/pub/png/libpng.html

#include "texture_libpng.h"
#include "scene/scene.h"
#include "png.h"

static void png_log(png_structp,png_const_charp msg) { nya_log::log(msg); }
static void png_no_log(png_structp,png_const_charp) {}

namespace nya_scene
{

bool load_texture_libpng(shared_texture &res,resource_data &data,const char* name)
{
    if(data.get_size()<8 || png_sig_cmp((png_bytep)data.get_data(),0,8)!=0)
        return false;

    png_structp png_ptr=png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,png_log,png_no_log);
    if(!png_ptr)
        return false;

    png_infop info_ptr=png_create_info_struct(png_ptr);
    if(!info_ptr)
    {
        png_destroy_read_struct(&png_ptr,NULL,NULL);
        return false;
    }

    if(setjmp(png_jmpbuf(png_ptr))!=0)
    {
        log()<<"unable to load texture with libpng file "<<name<<"\n";
        png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
        return false;
    }

    struct png_stream_reader
    {
        const resource_data &data;
        size_t offset;

        static void read(png_structp png_ptr,png_bytep buf,png_size_t size)
        {
            png_voidp io_ptr=png_get_io_ptr(png_ptr);
            if(!io_ptr)
                return;

            png_stream_reader *r=(png_stream_reader*)io_ptr;
            r->data.copy_to(buf,size,r->offset);
            r->offset+=size;
        }

        png_stream_reader(resource_data &data): data(data),offset(0) {}

    } png_reader(data);

    png_set_read_fn(png_ptr,&png_reader,png_stream_reader::read);

    png_read_info(png_ptr,info_ptr);

    const png_uint_32 width=png_get_image_width(png_ptr,info_ptr);
    const png_uint_32 height=png_get_image_height(png_ptr,info_ptr);
    const png_byte color_type=png_get_color_type(png_ptr,info_ptr);
    const png_byte bit_depth=png_get_bit_depth(png_ptr,info_ptr);

    if(!height || !width)
    {
        log()<<"unable to load texture with libpng: zero width or height in file "<<name<<"\n";
        png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
        return false;
    }

    if(bit_depth!=8)
    {
        if(bit_depth==16)
            png_set_strip_16(png_ptr);
        else if(bit_depth<8)
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        else
        {
            log()<<"unable to load texture with libpng: unsupported color depth: "<<bit_depth<<" in file "<<name<<"\n";
            png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
            return false;
        }
    }

    nya_render::texture::color_format cf;
    switch(color_type)
    {
        case PNG_COLOR_TYPE_GRAY: cf=nya_render::texture::greyscale; break;
        case PNG_COLOR_TYPE_RGB: cf=nya_render::texture::color_rgb; break;
        case PNG_COLOR_TYPE_RGB_ALPHA: cf=nya_render::texture::color_rgba; break;

        case PNG_COLOR_TYPE_PALETTE:
        {
            png_bytep trans_alpha=NULL;
            int num_trans=0;
            png_color_16p trans_color=NULL;
            png_get_tRNS(png_ptr,info_ptr,&trans_alpha,&num_trans,&trans_color);

            cf=trans_alpha?nya_render::texture::color_rgba:nya_render::texture::color_rgb;
            png_set_palette_to_rgb(png_ptr);
        }
        break;

        case PNG_COLOR_TYPE_GRAY_ALPHA:
            cf=nya_render::texture::color_rgba;
            png_set_gray_to_rgb(png_ptr);
            break;

        default: //ToDo
            log()<<"unable to load texture with libpng: unsupported color type: "<<color_type<<" in file "<<name<<"\n";
            png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
            return false;
    }

    png_read_update_info(png_ptr, info_ptr);

    if(setjmp(png_jmpbuf(png_ptr))!=0)
    {
        log()<<"unable to load texture with libpng file "<<name<<"\n";
        png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
        return false;
    }

    const png_size_t line_size=png_get_rowbytes(png_ptr,info_ptr);
    nya_memory::tmp_buffer_scoped image_data(line_size*height);
    std::vector<png_bytep> row_pointers(height);
    for(size_t i=0,off=0;i<row_pointers.size();++i,off+=line_size)
        row_pointers[row_pointers.size()-i-1]=(png_bytep)image_data.get_data(off);

    png_read_image(png_ptr,&row_pointers[0]);
    png_destroy_read_struct(&png_ptr,&info_ptr,NULL);

    res.tex.build_texture(image_data.get_data(),width,height,cf);
    texture::read_meta(res,data);
    return true;
}

}

namespace nya_formats
{

static void png_write_callback(png_structp  png_ptr,png_bytep data,png_size_t length)
{
    std::vector<char> &buf=*(std::vector<char>*)png_get_io_ptr(png_ptr);
    size_t off=buf.size();
    buf.resize(off+length);
    memcpy(&buf[off],data,length);
}

bool encode_png(const void *color_data,int width,int height,int channels,std::vector<char> &result)
{
    result.clear();

    if(!color_data || width<1 || height<1)
        return false;

    int color_type;
    switch(channels)
    {
        case 4: color_type=PNG_COLOR_TYPE_RGBA; break;
        case 3: color_type=PNG_COLOR_TYPE_RGB; break;
        case 1: color_type=PNG_COLOR_TYPE_GRAY; break;
        default: return false;
    }

    png_structp png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,png_log,png_no_log);
    if(!png_ptr)
        return false;

    png_infop info_ptr=png_create_info_struct(png_ptr);
    if(setjmp(png_jmpbuf(png_ptr))!=0)
    {
        png_destroy_write_struct(&png_ptr,&info_ptr);
        return false;
    }

    png_set_IHDR(png_ptr,info_ptr,width,height,8,color_type,
                 PNG_INTERLACE_NONE,PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);

    png_set_compression_level(png_ptr,2);
    png_set_compression_strategy(png_ptr,2);
    png_set_filter(png_ptr,PNG_FILTER_TYPE_BASE,PNG_FILTER_SUB);

    std::vector<png_bytep> rows(height);
    for(int i=0;i<height;++i)
        rows[i]=(png_bytep)color_data+(height-i-1)*width*channels;
    png_set_rows(png_ptr,info_ptr,&rows[0]);

    png_set_write_fn(png_ptr,&result,png_write_callback,NULL);
    png_write_png(png_ptr,info_ptr,PNG_TRANSFORM_IDENTITY,NULL);

    png_destroy_write_struct(&png_ptr,&info_ptr);
    return true;
}

}
