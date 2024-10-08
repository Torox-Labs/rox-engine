//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "dds.h"
#include "memory/memory_reader.h"
#include "memory/tmp_buffer.h"
#include "resources/resources.h"
#include <stdint.h>

namespace nya_formats
{

struct dds_pixel_format
{
    typedef uint32_t uint;

    uint size;
    uint flags;
    uint four_cc;
    uint bpp;
    uint bit_mask[4]; //rgba
};

static void flip_raw(int width,int height,int channels,const void *from_data,void *to_data)
{
    if(!height)
        return;

    const size_t line_size=width*channels, size=line_size*height;
    const unsigned char *from=(const unsigned char*)from_data;
    unsigned char *to=(unsigned char*)to_data+line_size*(height-1);
    for(size_t offset=0;offset<size;offset+=line_size)
        memcpy(to-offset,from+offset,line_size);
}

static void flip_dxt1_block_full(unsigned char *data)
{
    std::swap(data[4],data[7]);
    std::swap(data[5],data[6]);
}

static void flip_dxt3_block_full(unsigned char *data)
{
    std::swap(data[0],data[6]);
    std::swap(data[1],data[7]);
    std::swap(data[2],data[4]);
    std::swap(data[3],data[5]);

    flip_dxt1_block_full(data+8);
}

static void flip_dxt5_block_full(unsigned char *data)
{
    unsigned int line_0_1 = data[2] + 256 * (data[3] + 256 * data[4]);
    unsigned int line_2_3 = data[5] + 256 * (data[6] + 256 * data[7]);
    // swap lines 0 and 1 in line_0_1.
    unsigned int line_1_0 = ((line_0_1 & 0x000fff) << 12) |
    ((line_0_1 & 0xfff000) >> 12);
    // swap lines 2 and 3 in line_2_3.
    unsigned int line_3_2 = ((line_2_3 & 0x000fff) << 12) |
    ((line_2_3 & 0xfff000) >> 12);
    data[2] = line_3_2 & 0xff;
    data[3] = (line_3_2 & 0xff00) >> 8;
    data[4] = (line_3_2 & 0xff0000) >> 16;
    data[5] = line_1_0 & 0xff;
    data[6] = (line_1_0 & 0xff00) >> 8;
    data[7] = (line_1_0 & 0xff0000) >> 16;

    flip_dxt1_block_full(data+8);
}

static void flip_dxt(int width,int height,dds::pixel_format format,const void *from_data,void *to_data)
{
    if(from_data==to_data) //ToDo ?
        return;

    if(!height)
        return;

    const unsigned int line_size=((width+3)/4)*(format==dds::dxt1?8:16);
    const unsigned char *s=(unsigned char *)from_data;
    unsigned char *d=(unsigned char *)to_data+((height+3)/4-1)*line_size;

    if(height==1)
    {
        memcpy(d,s,line_size);
        return;
    }

    for(int i=0;i<(height+3)/4;++i)
    {
        memcpy(d,s,line_size);

        switch(format)
        {
            case dds::dxt1:
                if(height==2)
                {
                    for(unsigned int k=0;k<line_size;k+=8)
                        std::swap((d+k)[4],(d+k)[5]);
                }
                else
                {
                    for(unsigned int k=0;k<line_size;k+=8)
                        flip_dxt1_block_full(d+k);
                }
                break;

            case dds::dxt2:
            case dds::dxt3:
                for(unsigned int k=0;k<line_size;k+=16)
                    flip_dxt3_block_full(d+k);
                break;

            case dds::dxt4:
            case dds::dxt5:
                for(unsigned int k=0;k<line_size;k+=16)
                    flip_dxt5_block_full(d+k);
                break;

            default: return;
        }

        s+=line_size;
        d-=line_size;
    }
}

void dds::flip_vertical(const void *from_data,void *to_data) const
{
    if(!from_data || !to_data || !height)
        return;

    if(type==texture_cube) //ToDo
        return;

    size_t offset=0;
    for(unsigned int i=0,w=width,h=height;i<mipmap_count;++i,w>1?w=w/2:w=1,h>1?h/=2:h=1)
    {
        const void *f=(const char *)from_data+offset;
        void *t=(char *)to_data+offset;

        switch(pf)
        {
            case rgb:
            case bgr:
            case rgba:
            case bgra:
            case greyscale:
            {
                const int channels=(pf==bgra||pf==rgba)?4:pf==(pf==bgr||pf==rgb)?3:1;
                flip_raw(w,h,channels,f,t);
                offset+=w*h*(channels);
            }
            break;

            case dxt1:
            {
                unsigned int s=(w>4?w:4)/4 * (h>4?h:4)/4 * 8;
                flip_dxt(w,h,pf,f,t);
                offset+=s;
            }
            break;

            case dxt2:
            case dxt3:
            case dxt4:
            case dxt5:
            {
                unsigned int s=(w>4?w:4)/4 * (h>4?h:4)/4 * 16;
                flip_dxt(w,h,pf,f,t);
                offset+=s;
            }
            break;

            case palette4_rgba:
            case palette8_rgba:
            {
                return; //ToDo: not tested

                const int offset=pf==palette8_rgba?256*4:16*4;
                flip_raw(w,h,1,(const char *)f+offset,(char *)t+offset);
            }
            break;
        }
    }
}

size_t dds::get_decoded_size() const
{
    size_t size=0;
    for(unsigned int i=0,w=width,h=height;i<mipmap_count;++i,w>1?w=w/2:w=1,h>1?h/=2:h=1)
        size+=w*h*4;

    return type==texture_cube?size*6:size;
}

void dds::decode_palette8_rgba(void *decoded_data) const
{
    if(pf!=palette8_rgba)
        return;

    //used memcpy instead of cast to avoid misalignment
    unsigned int palette[256];
    memcpy(palette,data,sizeof(palette));

    const unsigned char *inds=(unsigned char *)data+sizeof(palette);
    unsigned char *out=(unsigned char *)decoded_data; //alignment
    for(unsigned int i=0;i<width*height;++i,++inds,out+=4)
        memcpy(out,&palette[*inds],4);
}

inline int unpack565(const unsigned char* src,unsigned char* dst)
{
    typedef unsigned char uchar;

    int value=(int)src[0] | ((int)src[1]<<8);

    const uchar r=(uchar)((value >> 11) & 0x1f);
    const uchar g=(uchar)((value >> 5) & 0x3f);
    const uchar b=(uchar)(value & 0x1f);

    dst[0]=(r << 3) | (r >> 2);
    dst[1]=(g << 2) | (g >> 4);
    dst[2]=(b << 3) | (b >> 2);
    dst[3]=255;

    return value;
}

inline void decompress_color(const void *src,void *dst,bool is_dxt1)
{
    typedef unsigned char uchar;

    const uchar* src_buf=(uchar *)src;
    if(!is_dxt1)
        src_buf+=8;

    uchar codes[16];
    const int a=unpack565(src_buf,codes), b=unpack565(src_buf+2,codes+4);

    for(int i=0;i<3;++i)
    {
        const int c=codes[i], d=codes[i+4];

        if(is_dxt1 && a<=b)
        {
            codes[i+8]=(uchar)((c+d)/2);
            codes[i+12]=0;
        }
        else
        {
            codes[i+8]=(uchar)((c*2+d)/3);
            codes[i+12]=(uchar)((c+d*2)/3);
        }
    }

    codes[8+3]=255;
    codes[12+3]=(is_dxt1 && a<=b)?0:255;

    uchar indices[16];
    for(int i=0;i<4;++i)
    {
        const uchar packed=src_buf[i+4];
        uchar* ind=indices + i*4;

        ind[0]=packed & 0x3;
        ind[1]=(packed >> 2) & 0x3;
        ind[2]=(packed >> 4) & 0x3;
        ind[3]=(packed >> 6) & 0x3;
    }

    for(int i=0;i<16;++i)
    {
        const uchar offset=indices[i]*4;
        for(int j=0;j<4;++j)
            ((uchar *)dst)[i*4 + j]=codes[offset+j];
    }
}

inline void decompress_dxt3_alpha(const void *src,void *dst)
{
    typedef unsigned char uchar;

    const uchar *src_buf=(uchar *)src;
    uchar *dst_buf=(uchar *)dst;

    for(int i=0;i<8*8;i+=8,++src_buf)
    {
        const uchar lo= *src_buf & 0x0f, hi= *src_buf & 0xf0;
        dst_buf[i+3] = lo | (lo<<4);
        dst_buf[i+7] = hi | (hi>>4);
    }
}

inline void decompress_dxt5_alpha(const void *src,void *dst)
{
    typedef unsigned char uchar;

    const uchar *src_buf=(uchar *)src;
    uchar *dst_buf=(uchar *)dst;

    const int alpha0=src_buf[0], alpha1=src_buf[1];

    uchar codes[8];
    codes[0]=src_buf[0], codes[1]=src_buf[1];
    if(alpha0<=alpha1)
    {
        for(int i=1;i<5;++i)
            codes[i+1]=(uchar)(((5-i)*alpha0 + i*alpha1 )/5);

        codes[6]=0, codes[7]=255;
    }
    else
    {
        for(int i=1;i<7;++i)
            codes[i+1]=(uchar)(((7-i)*alpha0 + i*alpha1 )/7);
    }

    src_buf+=2;
    uchar indices[16];
    uchar* dest=indices;
    for(int i=0;i<2;++i)
    {
        unsigned int value=0;
        for(int j=0;j<3;++j)
            value|=((*src_buf++) << 8*j);

        for(int j=0;j<8;++j)
            *dest++ = (uchar)(value >> 3*j) & 0x7;
    }

    for(int i=0;i<16;++i)
        dst_buf[4*i+3]=codes[indices[i]];
}

void dds::decode_dxt(void *decoded_data) const
{
    typedef unsigned int uint;
    const char* src_buf=(char *)data;
    const uint bpb=pf==dxt1?8:16;

    for(int f=0;f<(type==texture_cube?6:1);++f)
    {
        for(uint i=0,w=width,h=height;i<mipmap_count;++i,w>1?w/=2:w=1,h>1?h/=2:h=1)
        {
            for(uint y=0;y<h;y+=4) for(uint x=0;x<w;x+=4)
            {
                uint rgba[16];

                switch(pf)
                {
                    case dxt1: decompress_color(src_buf,rgba,true); break;

                    case dxt2:
                    case dxt3:
                        decompress_color(src_buf,rgba,false);
                        decompress_dxt3_alpha(src_buf,rgba);
                        break;

                    case dxt4:
                    case dxt5:
                        decompress_color(src_buf,rgba,false);
                        decompress_dxt5_alpha(src_buf,rgba);
                        break;

                    default: return;
                }

                for(uint py=0,sy=y; py<16 && sy<h; py+=4,++sy)
                    memcpy((uint *)decoded_data+w*sy+x,&rgba[py],((x+4<w)?4:(w-x))*sizeof(uint));
                
                src_buf+=bpb;
            }

            decoded_data=(char *)decoded_data+(w*h)*4;
        }
    }
}

size_t dds::decode_header(const void *data,size_t size)
{
    *this=dds();

    if(!data || size<128)
        return 0;

    typedef uint32_t uint;

    nya_memory::memory_reader reader(data,size);
    if(!reader.test("DDS ",4))
        return 0;

    if(reader.read<uint>()!=124)
        return 0;

    const uint dds_pixelformat=0x00001000;
    const uint dds_caps=0x00000001;

    const uint flags=reader.read<uint>();
    if(!(flags & dds_pixelformat) || !(flags & dds_caps))
        return 0;

    height=reader.read<uint>();
    width=reader.read<uint>();
    reader.skip(4); //const uint pitch=reader.read<uint>();
    const uint depth=reader.read<uint>();
    if(depth>1) //ToDo: 3d textures
        return 0;

    mipmap_count=reader.read<uint>();
    if(!mipmap_count)
    {
        need_generate_mipmaps=true;
        mipmap_count=1;
    }

    reader.skip(44);

    const uint dds_fourcc=0x00000004;
    //const uint dds_rgb=0x00000040;
    //const uint dds_alpha=0x00000001;
    const uint dds_palette4=0x00000008;
    const uint dds_palette8=0x00000020;
    const uint dds_cubemap=0x00000200;

    const dds_pixel_format pf=reader.read<dds_pixel_format>();
    if(pf.flags & dds_fourcc)
    {
        switch(pf.four_cc)
        {
            case 0x31545844 /*'1TXD'*/ :this->pf=dxt1; break;
            case 0x32545844 /*'2TXD'*/ :this->pf=dxt2; break;
            case 0x33545844 /*'3TXD'*/ :this->pf=dxt3; break;
            case 0x34545844 /*'4TXD'*/ :this->pf=dxt4; break;
            case 0x35545844 /*'5TXD'*/ :this->pf=dxt5; break;
            default: return 0;
        };

        for(uint i=0,w=width,h=height;i<mipmap_count;++i,w/=2,h/=2)
        {
            uint s=(w>4?w:4)/4 * (h>4?h:4)/4 * (this->pf==dxt1?8:16);
            this->data_size+=s;
        }
    }
    else
    {
        if(pf.bpp==32)
        {
            //if(!(pf.flags & dds_alpha) || pf.bit_mask[3]!=0xff000000U)) //load anyway
            //    return 0;

            //if(!(pf.flags & dds_rgb) || pf.bit_mask[0]!=0xff0000 || pf.bit_mask[1]!=0xff00 || pf.bit_mask[2]!=0xff )
            //    return 0;

            this->pf=pf.bit_mask[0]==255?rgba:bgra;
        }
        else if(pf.bpp==24)
            this->pf=this->pf=pf.bit_mask[0]==255?rgb:bgr;
        else if(pf.bpp==8)
        {
            if(pf.flags & dds_palette8)
                this->pf=palette8_rgba;
            else if(pf.flags & dds_palette4)
                this->pf=palette4_rgba;
            else
                this->pf=greyscale;
        }
        else
            return 0;

        for(uint i=0,w=width,h=height;i<mipmap_count;++i,w>1?w=w/2:w=1,h>1?h/=2:h=1)
            this->data_size+=w*h*(pf.bpp/8);
    }

    reader.read<uint>(); //caps
    const uint caps2=reader.read<uint>();

    type=texture_2d;
    if(caps2 & dds_cubemap)
    {
        type=texture_cube;
        this->data_size*=6;
    }

    reader.seek(128);
    if(!reader.check_remained(this->data_size))
    {
        this->mipmap_count= -1;
        //probably broken, try load at least first mipmap
        if(!reader.check_remained(this->data_size))
            return 0;
    }

    this->data=reader.get_data();

    return reader.get_offset();
}

size_t dds::get_mip_size(int mip_idx) const
{
    if(mip_idx<0 || mip_idx>=int(mipmap_count))
       return 0;

    const unsigned int w1=width>>mip_idx, h1=height>>mip_idx;
    if(pf<=dxt5)
        return (w1>4?w1:4)/4 * (h1>4?h1:4)/4 * (pf==dxt1?8:16) * (type==texture_cube?6:1);

    return (w1>1?w1:1) * (h1>1?h1:1) * (pf==bgra?4:(pf==bgr?3:1)) * (type==texture_cube?6:1);
}

}
