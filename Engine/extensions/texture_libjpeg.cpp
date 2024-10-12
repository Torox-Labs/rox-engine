//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//requires: libjpeg http://ijg.org/

#include "texture_libjpeg.h"
#include "scene/scene.h"
#include "jpeglib.h"

namespace nya_scene
{

bool load_texture_libjpeg(shared_texture &res,resource_data &data,const char* name)
{
    const unsigned char jpg_sign[]={0xff,0xd8,0xff};

    if(data.get_size()<3 || memcmp(data.get_data(),jpg_sign,3)!=0)
        return false;

    nya_formats::jpeg jpeg;
    if(!jpeg.decode_header(data.get_data(),data.get_size()))
    {
        log()<<"unable to load texture with libjpeg: invalid header "<<name<<"\n";
        return false;
    }

    if(!jpeg.width || !jpeg.height)
    {
        log()<<"unable to load texture with libjpeg: zero width or height in file "<<name<<"\n";
        return false;
    }

    nya_render::texture::color_format cf;
    switch(jpeg.channels)
    {
        case nya_formats::jpeg::rgb: cf=nya_render::texture::color_rgb; break;
        case nya_formats::jpeg::greyscale: cf=nya_render::texture::greyscale; break;

        default:
            log()<<"unable to load texture with libjpeg: unsupported "<<jpeg.channels<<" components in file "<<name<<"\n";
            return false;
    }

    nya_memory::tmp_buffer_scoped buf(jpeg.get_decoded_size());
    if(!jpeg.decode_color(buf.get_data()))
        return false;

    res.tex.build_texture(buf.get_data(),jpeg.width,jpeg.height,cf);
    texture::read_meta(res,data);
    return true;
}

}

namespace
{

struct source_memory: public jpeg_source_mgr
{
    static boolean on_fill_input_buffer(j_decompress_ptr cinfo)
    {
        const static JOCTET eoi[]={JPEG_EOI};
        cinfo->src->next_input_byte=eoi;
        cinfo->src->bytes_in_buffer=1;
        return TRUE;
    }

    static void on_skip_input_data(j_decompress_ptr cinfo,long num_bytes)
    {
        if(cinfo->src->bytes_in_buffer<(size_t)num_bytes)
        {
            const static JOCTET eoi[]={JPEG_EOI};
            cinfo->src->next_input_byte=eoi;
            cinfo->src->bytes_in_buffer=1;
        }
        else
        {
            cinfo->src->next_input_byte+=num_bytes;
            cinfo->src->bytes_in_buffer-=num_bytes;
        }
    }

    static void unused(j_decompress_ptr cinfo) {}

    source_memory(const void *data,size_t size)
    {
        init_source=unused;
        fill_input_buffer=on_fill_input_buffer;
        skip_input_data=on_skip_input_data;
        resync_to_restart=jpeg_resync_to_restart;
        term_source=unused;
        bytes_in_buffer=size;
        next_input_byte=(JOCTET *)data;
    }
};

struct dest_memory: public jpeg_destination_mgr
{
    static void mem_init_destination(j_compress_ptr cinfo)
    {
        dest_memory* dst=(dest_memory*)cinfo->dest;
        dst->buffer.resize(block_size);
        cinfo->dest->next_output_byte=(JOCTET *)dst->buffer.data();
        cinfo->dest->free_in_buffer=dst->buffer.size();
    }

    static void mem_term_destination(j_compress_ptr cinfo)
    {
        dest_memory* dst=(dest_memory*)cinfo->dest;
        dst->buffer.resize(dst->buffer.size() - cinfo->dest->free_in_buffer);
    }

    static boolean mem_empty_output_buffer(j_compress_ptr cinfo)
    {
        dest_memory* dst=(dest_memory*)cinfo->dest;
        size_t offset=dst->buffer.size();
        dst->buffer.resize(offset+block_size);
        cinfo->dest->next_output_byte=(JOCTET *)dst->buffer.data()+offset;
        cinfo->dest->free_in_buffer=block_size;
        return TRUE;
    }

    dest_memory(std::vector<char> &buf):buffer(buf)
    {
        init_destination=mem_init_destination;
        term_destination=mem_term_destination;
        empty_output_buffer=mem_empty_output_buffer;
    }

private:
    std::vector<char> &buffer;
    static const int block_size=16384;
};

struct error_mgr: public jpeg_error_mgr
{
    static void unused(j_common_ptr cinfo) {}
    error_mgr() { jpeg_std_error(this); error_exit=unused; }
};

}

namespace nya_formats
{

bool jpeg::decode_header(const void *data,size_t size)
{
    *this=jpeg();

    if(!data)
        return false;

    const unsigned char jpg_sign[]={0xff,0xd8,0xff};
    if(size<3 || memcmp(data,jpg_sign,3)!=0)
        return false;

    jpeg_decompress_struct cinfo;
    memset(&cinfo,0,sizeof(cinfo));
    error_mgr err;
    cinfo.err=&err;
    jpeg_create_decompress(&cinfo);

    source_memory src(data,size);
    cinfo.src=&src;

    if(jpeg_read_header(&cinfo,TRUE)!=JPEG_HEADER_OK)
    {
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    width=cinfo.image_width;
    height=cinfo.image_height;
    channels=cinfo.num_components==1?greyscale:rgb;

    jpeg_destroy_decompress(&cinfo);

    this->data=data;
    data_size=size;
    return true;
}

bool jpeg::decode_color(void *decoded_data) const
{
    if(!decoded_data || !data)
        return false;

    jpeg_decompress_struct cinfo;
    memset(&cinfo,0,sizeof(cinfo));
    error_mgr err;
    cinfo.err=&err;
    jpeg_create_decompress(&cinfo);

    source_memory src(data,data_size);
    cinfo.src=&src;

    if(jpeg_read_header(&cinfo,TRUE)!=JPEG_HEADER_OK)
    {
        jpeg_destroy_decompress(&cinfo);
        return false;
    }

    jpeg_start_decompress(&cinfo);

    const size_t stride=cinfo.output_width*cinfo.out_color_components;
    while(cinfo.output_scanline<cinfo.output_height)
    {
        unsigned char *rowp[]={(unsigned char *)decoded_data+stride*(cinfo.output_height-cinfo.output_scanline-1)};
        jpeg_read_scanlines(&cinfo,rowp,1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    return true;
}

size_t jpeg::get_decoded_size() const { return width*height*channels; }

bool encode_jpeg(const void *color_data,int width,int height,int channels,std::vector<char> &result,int quality)
{
    if(!color_data || width<1 || height<1 || quality<=0 || quality >100)
        return false;

    J_COLOR_SPACE colorspace;
    switch(channels)
    {
        case 1: colorspace=JCS_GRAYSCALE; break;
        case 3: colorspace=JCS_RGB; break;
        default: return false;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err=jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    dest_memory dst(result);
    cinfo.dest=&dst;

    cinfo.image_width=width;
    cinfo.image_height=height;
    cinfo.input_components=channels;
    cinfo.in_color_space=colorspace;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo,quality,TRUE);
    jpeg_set_colorspace(&cinfo,colorspace);
    cinfo.dct_method=JDCT_IFAST;

    jpeg_start_compress(&cinfo,TRUE);

    const size_t stride=cinfo.image_width*cinfo.input_components;
    while(cinfo.next_scanline<cinfo.image_height)
    {
        JSAMPROW rowp[]={(JSAMPROW)color_data+stride*(cinfo.image_height-cinfo.next_scanline-1)};
        jpeg_write_scanlines(&cinfo,rowp,1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return true;
}

}
