//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "tga.h"
#include "memory/memory_reader.h"
#include "memory/tmp_buffer.h"
#include "resources/resources.h"
#include <stdio.h>

namespace nya_formats
{

size_t tga::decode_header(const void *data,size_t size)
{
    *this=tga();

    if(!data || !size)
        return 0;

    nya_memory::memory_reader reader(data,size);

    const char id_length=reader.read<char>();
    if(!reader.skip(id_length))
        return 0;

    const char colourmaptype=reader.read<char>();
    if(colourmaptype!=0)
        return 0;

    const char datatypecode=reader.read<char>();
    const short colourmaporigin=reader.read<short>();
    if(colourmaporigin!=0)
        return 0;

    const short colourmaplength=reader.read<short>();
    if(colourmaplength!=0)
        return 0;

    const char colourmapdepth=reader.read<char>();
    if(colourmapdepth!=0)
        return 0;

    //const short x_origin=
    reader.read<short>();
    //const short y_origin=
    reader.read<short>();

    const short width=reader.read<short>();
    const short height=reader.read<short>();
    const char bitsperpixel=reader.read<char>();
    const unsigned char imagedescriptor=reader.read<char>();

    color_mode channels;
    bool rle=false;

    switch(bitsperpixel)
    {
        case 32:
            if(datatypecode==10)
                channels=bgra,rle=true;
            else if(datatypecode==2)
                channels=bgra;
            else
                return false;
        break;

        case 24:
            if(datatypecode==10)
                channels=bgr,rle=true;
            else if(datatypecode==2)
                channels=bgr;
            else
                return false;
        break;

        case 8:
            if(datatypecode==11)
                channels=greyscale,rle=true;
            else if(datatypecode==3)
                channels=greyscale;
            else
                return false;
        break;

        default:
            return 0;
    }

    const size_t color_data_size=width*height*channels;
    if(!color_data_size)
        return 0;

    this->width=width;
    this->height=height;
    this->channels=channels;
    this->rle=rle;
    this->horisontal_flip=(imagedescriptor&0x10)!=0;
    this->vertical_flip=(imagedescriptor&0x20)!=0;
    this->data=reader.get_data();
    this->compressed_size=reader.get_remained();
    this->uncompressed_size=color_data_size;

    return reader.get_offset();
}

size_t tga::encode_header(void *to_data,size_t to_size)
{
    if(to_size<tga_header_size)
        return 0;

    char datatypecode=0;
    char bpp=channels*8;

    datatypecode=rle?10:2;
    if(channels==1)
        ++datatypecode;

    if(!datatypecode)
        return 0;

    unsigned char imagedescriptor=0;
    if(horisontal_flip)
        imagedescriptor|=0x10;
    if(vertical_flip)
        imagedescriptor|=0x20;

    unsigned short w=width,h=height;
    char *out=(char *)to_data;
    memset(out,0,tga_header_size);
    memcpy(out+2,&datatypecode,1);
    memcpy(out+12,&w,2);
    memcpy(out+14,&h,2);
    memcpy(out+16,&bpp,1);
    memcpy(out+17,&imagedescriptor,1);

    return tga_header_size;
}

bool tga::decode_rle(void *decoded_data) const
{
    if(!decoded_data || !rle)
        return false;

    typedef unsigned char uchar;
    const uchar *cur=(uchar*)data;
    const uchar *const last=(uchar*)data+compressed_size;
    uchar *out=(uchar*)decoded_data;
    const uchar *const out_last=out+uncompressed_size;

    while(out<out_last)
    {
        if(cur>=last)
            return false;

        if(*cur & 0x80)
        {
            const uchar *to=out+(*cur++ -127)*channels;
            if(cur+channels>last || to>out_last)
                return false;

            while(out<to) memcpy(out,cur,channels),out+=channels;
            cur+=channels;
        }
        else // raw
        {
            const size_t size=(*cur++ +1)*channels;
            if(cur+size>last || out+size>out_last)
                return false;

            memcpy(out,cur,size);
            cur+=size,out+=size;
        }
    }

    return true;
}

size_t tga::encode_rle(void *to_data,size_t to_size)
{
    typedef unsigned char uchar;

    const uchar *from=(uchar *)data;
    const uchar *from_last=from+uncompressed_size;
    uchar *to=(uchar *)to_data;
    uchar *to_last=to+to_size;

    uchar raw[128*4];
    memset(raw,0,sizeof(raw));

    int curr_line=0;
    for(int rle=1;from<from_last;from+=channels*rle,curr_line+=rle)
    {
        if(curr_line>=width)
            curr_line-=width;

        memcpy(raw,from,channels);

        rle=1;
        bool is_rle=false;
        int max_rle=128;
        if(curr_line+max_rle>width)
            max_rle=width-curr_line;

        for(const uchar *check=from+channels;check<from_last;++rle,check+=channels)
        {
            if(memcmp(raw,check,channels)!=0 || rle>=max_rle)
            {
                is_rle=rle>1;
                break;
            }
        }

        if(is_rle)
        {
            if(to+channels+1>to_last)
                return 0;

            *to++ =(128 | (rle-1));
            memcpy(to,raw,channels);
            to+=channels;
            continue;
        }

        rle = 1;
        uchar *raw_it=raw;
        for(const uchar *check=from+channels;check<from_last;++rle,check+=channels)
        {
            if((memcmp(raw_it,check,channels)!=0 && rle<max_rle) || rle<3)
            {
                memcpy(raw_it+=channels,check,channels);
                if(rle>=max_rle)
                    break;

                continue;
            }

            // check if the exit condition was the start of a repeating color
            if(memcmp(raw_it,check,channels)!=0)
                rle-=2;

            break;
        }

        size_t raw_size=channels*rle;
        if(to+raw_size+1>to_last)
            return 0;

        *to++ =rle-1;
        memcpy(to,raw,raw_size);
        to+=raw_size;
    }

    return to-(uchar*)to_data;
}

void tga::flip_vertical(const void *from_data,void *to_data)
{
    if(!from_data || !to_data || !height)
        return;

    const int line_size=width*channels, top=line_size*(height-1);

    typedef unsigned char uchar;

    uchar *to=(uchar*)to_data;

    if(from_data==to_data)
    {
        if(!line_size)
            return;

        const int half=line_size*(height/2);
        std::vector<uchar> line_data(line_size);
        uchar *line=&line_data[0];
        uchar *from=to;
        to+=top;

        for(int offset=0;offset<half;offset+=line_size)
        {
            uchar *f=from+offset, *t=to-offset;
            memcpy(line,f,line_size);
            memcpy(f,t,line_size);
            memcpy(t,line,line_size);
        }
    }
    else
    {
        const uchar *from=(const uchar*)from_data;
        to+=top;
        for(size_t offset=0;offset<uncompressed_size;offset+=line_size)
            memcpy(to-offset,from+offset,line_size);
    }
}

void tga::flip_horisontal(const void *from_data,void *to_data)
{
    if(!from_data || !to_data)
        return;

    const int line_size=width*channels;
    const int half=line_size/2;

    typedef unsigned char uchar;

    uchar *to=(uchar*)to_data;

    if(from_data==to_data)
    {
        uchar tmp[4];

        for(size_t offset=0;offset<uncompressed_size;offset+=line_size)
        {
            uchar *ha=to+offset;
            uchar *hb=ha+line_size-channels;

            for(int w=0;w<half;w+=channels)
            {
                uchar *a=ha+w;
                uchar *b=hb-w;
                memcpy(tmp,a,channels);
                memcpy(a,b,channels);
                memcpy(b,tmp,channels);
            }
        }
    }
    else
    {
        const uchar *from=(const uchar*)from_data;

        for(size_t offset=0;offset<uncompressed_size;offset+=line_size)
        {
            const uchar *ha=from+offset;
            uchar *hb=to+offset+line_size-channels;

            for(int w=0;w<line_size;w+=channels)
            {
                const uchar *a=ha+w;
                uchar *b=hb-w;
                memcpy(b,a,channels);
            }
        }
    }
}

bool tga_file::load(const char *file_name)
{
    release();

    nya_resources::resource_data *in_data=nya_resources::get_resources_provider().access(file_name);
    if(!in_data)
    {
        printf( "unable to open texture %s\n", file_name );
        return false;
    }

    nya_memory::tmp_buffer_scoped in_buf(in_data->get_size());
    in_data->read_all(in_buf.get_data());
    const bool header_decoded=m_header.decode_header(in_buf.get_data(),in_data->get_size())>0;
    in_data->release();
    if(!header_decoded)
        return false;

    if(m_header.rle)
    {
        m_data.resize(m_header.compressed_size);
        memcpy(&m_data[0],m_header.data,m_header.compressed_size);
    }
    else
    {
        m_data.resize(m_header.uncompressed_size);
        memcpy(&m_data[0],m_header.data,m_header.uncompressed_size);
    }

    return true;
}

bool tga_file::create(int width,int height,tga::color_mode channels,const void *data)
{
    release();

    if(width<=0 || height<=0)
        return false;

    m_header.width=width;
    m_header.height=height;
    m_header.channels=channels;

    m_data.resize(width*height*channels,0);
    if(data)
        memcpy(&m_data[0],data,m_data.size());

    return true;
}

bool tga_file::decode_rle()
{
    if(m_data.empty())
        return false;

    if(!m_header.rle)
        return false;

    m_header.data=&m_data[0];
    nya_memory::tmp_buffer_scoped buf(m_header.uncompressed_size);
    if(!m_header.decode_rle(buf.get_data()))
        return false;

    m_header.rle=false;
    m_data.resize(m_header.uncompressed_size);
    memcpy(&m_data[0],buf.get_data(),m_header.uncompressed_size);

    return true;
}

bool tga_file::encode_rle(size_t max_compressed_size)
{
    if(m_data.empty())
        return false;

    if(m_header.rle)
        return false;

    m_header.data=&m_data[0];
    nya_memory::tmp_buffer_scoped buf(max_compressed_size);
    size_t encoded_size=m_header.encode_rle(buf.get_data(),max_compressed_size);
    if(!encoded_size)
        return false;

    m_header.rle=true;
    m_header.compressed_size=encoded_size;
    m_data.resize(encoded_size);
    memcpy(&m_data[0],buf.get_data(),encoded_size);

    return true;
}

bool tga_file::save(const char *file_name)
{
    if(m_data.empty())
        return false;

    FILE *out_file=fopen( file_name, "wb" );
    if(!out_file)
    {
        printf( "unable to save texture %s\n", file_name );
        return false;
    }

    char header_buf[m_header.tga_header_size];
    size_t header_size=m_header.encode_header(header_buf,sizeof(header_buf));
    fwrite(header_buf,1,header_size,out_file);
    fwrite(&m_data[0],1,m_data.size(),out_file);

    fclose(out_file);
    return true;
}

bool tga_file::flip_horisontal()
{
    if(m_data.empty())
        return false;

    if(m_header.rle && !decode_rle())
        return false;

    m_header.flip_horisontal(&m_data[0],&m_data[0]);
    m_header.horisontal_flip=!m_header.horisontal_flip;

    return true;
}

bool tga_file::flip_vertical()
{
    if(m_data.empty())
        return false;

    if(m_header.rle && !decode_rle())
        return false;

    m_header.flip_vertical(&m_data[0],&m_data[0]);
    m_header.vertical_flip=!m_header.vertical_flip;

    return true;
}

}
