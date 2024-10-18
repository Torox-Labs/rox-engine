//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "bitmap.h"
#include "memory/align_alloc.h"
#include "memory/tmp_buffer.h"
#include <string.h>
#include <vector>

namespace nya_render
{

inline uint32_t average(uint32_t a, uint32_t b) { return (((a^b) & 0xfefefefeL)>>1) + (a&b); }

void bitmap_downsample_x(const uint32_t *data32,int width,int height,int channels,uint32_t *out32)
{
    for(int h=0;h<height;++h)
    {
        const uint32_t *hdata=data32+h*width;
        for(int w=0;w<width/2;++w,hdata+=2)
            *out32++ = average(hdata[0],hdata[1]);
    }
}

void bitmap_downsample_x(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    for(int h=0;h<height;++h)
    {
        const uint8_t *hdata=data+(h*channels)*width;
        for(int w=0;w<width/2;++w,hdata+=channels*2)
        {
            const uint8_t *wdata=hdata;
            for(int c=0;c<channels;++c,++wdata)
                *out++ =(wdata[0]+wdata[channels])/2;
        }
    }
}

void bitmap_downsample_y(const uint32_t *data32,int width,int height,int channels,uint32_t *out32)
{
    for(int h=0;h<height/2;++h)
    {
        const uint32_t *hdata=data32+h*width*2;
        for(int w=0;w<width;++w,++hdata)
            *out32++ = average(hdata[0],hdata[width]);
    }
}

void bitmap_downsample_y(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    const int stride=channels*width;
    for(int h=0;h<height/2;++h)
    {
        const uint8_t *hdata=data+h*stride*2;
        for(int w=0;w<width;++w,hdata+=channels)
        {
            const uint8_t *wdata=hdata;
            for(int c=0;c<channels;++c,++wdata)
                *out++ =(wdata[0] + wdata[stride])/2;
        }
    }
}

void bitmap_downsample2x(uint8_t *data,int width,int height,int channels)
{
    if(channels==4 && nya_memory::is_aligned(data,4))
    {
        if(width>1)
        {
            bitmap_downsample_x((uint32_t *)data,width,height,channels,(uint32_t *)data);
            width/=2;
        }
        if(height>1)
            bitmap_downsample_y((uint32_t *)data,width,height,channels,(uint32_t *)data);
    }
    else
    {
        if(width>1)
        {
            bitmap_downsample_x(data,width,height,channels,data);
            width/=2;
        }
        if(height>1)
            bitmap_downsample_y(data,width,height,channels,data);
    }
}

void bitmap_downsample2x(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    if(width<=1)
    {
        if(height>1)
            bitmap_downsample_y(data,width,height,channels,out);
        return;
    }

    if(height<=1)
    {
        if(width>1)
            bitmap_downsample_x(data,width,height,channels,out);
        return;
    }

    if(channels==4 && nya_memory::is_aligned(data,4))
    {
        const uint32_t *data32=(uint32_t *)data;
        uint32_t *out32=(uint32_t *)out;
        for(int h=0;h<height/2;++h)
        {
            const uint32_t *hdata=data32+(h*2)*width;
            for(int w=0;w<width/2;++w,hdata+=2)
                *out32++ = average(average(hdata[0],hdata[1]),average(hdata[width],hdata[width+1]));
        }
        return;
    }

    for(int h=0;h<height/2;++h)
    {
        const uint8_t *hdata=data+(h*channels*2)*width;
        for(int w=0;w<width/2;++w,hdata+=channels*2)
        {
            const uint8_t *wdata=hdata;
            for(int c=0;c<channels;++c,++wdata)
            {
                *out++ =(wdata[0]+
                         wdata[channels]+
                         wdata[channels*width]+
                         wdata[channels*width+channels])/4;
            }
        }
    }
}

inline void swap(uint8_t *a,uint8_t *b,int channels)
{
    uint8_t tmp;
    for(int i=0;i<channels;++i)
        tmp=a[i],a[i]=b[i],b[i]=tmp;
}

void bitmap_flip_vertical(uint8_t *data,int width,int height,int channels)
{
    const int line_size=width*channels;
    const int half=line_size*(height/2);
    const int top=line_size*(height-1);
    std::vector<uint8_t> line_data(line_size);
    uint8_t *line=&line_data[0], *from=data, *to=data+top;

    for(int offset=0;offset<half;offset+=line_size)
    {
        uint8_t *f=from+offset, *t=to-offset;
        memcpy(line,f,line_size);
        memcpy(f,t,line_size);
        memcpy(t,line,line_size);
    }
}

void bitmap_flip_vertical(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    const int line_size=width*channels;
    out+=line_size*(height-1);
    for(int y=0;y<height;++y,data+=line_size,out-=line_size)
        memcpy(out,data,line_size);
}

void bitmap_flip_horisontal(uint8_t *data,int width,int height,int channels)
{
    const int line_size=width*channels;
    const int size=width*height*channels;
    const int half=line_size/2;

    for(int offset=0;offset<size;offset+=line_size)
    {
        uint8_t *ha=data+offset;
        uint8_t *hb=ha+line_size-channels;

        for(int w=0;w<half;w+=channels)
            swap(ha+w,hb-w,channels);
    }
}

void bitmap_flip_horisontal(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    const int line_size=width*channels;
    out+=(width-1)*channels;
    for(int y=0;y<height;++y,data+=line_size,out+=line_size)
    {
        for(int w=0;w<line_size;w+=channels)
            memcpy(out-w,data+w,channels);
    }
}

void bitmap_rotate_90_left(uint8_t *data,int width,int height,int channels)
{
    if(width!=height)
    {
        nya_memory::tmp_buffer_scoped buf(width*height*channels);
        bitmap_rotate_90_left(data,width,height,channels,(uint8_t *)buf.get_data());
        buf.copy_to(data,buf.get_size());
        return;
    }

    for(int y=0;y<width;++y)
    {
        for(int x=y+1;x<width;++x)
            swap(data+(y*width+x)*channels,data+(x*width+y)*channels,channels);
    }

    bitmap_flip_horisontal(data,width,height,channels);
}

void bitmap_rotate_90_left(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    for(int x=0;x<height;++x)
    {
        for(int y=0;y<width;++y,data+=channels)
            memcpy(out+(y*height+height-x-1)*channels,data,channels);
    }
}

void bitmap_rotate_90_right(uint8_t *data,int width,int height,int channels)
{
    if(width!=height)
    {
        nya_memory::tmp_buffer_scoped buf(width*height*channels);
        bitmap_rotate_90_right(data,width,height,channels,(uint8_t *)buf.get_data());
        buf.copy_to(data,buf.get_size());
        return;
    }

    for(int y=0;y<width;++y)
    {
        for(int x=y+1;x<width;++x)
            swap(data+(y*width+x)*channels,data+(x*width+y)*channels,channels);
    }

    bitmap_flip_vertical(data,width,height,channels);
}

void bitmap_rotate_90_right(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    for(int x=0;x<height;++x)
    {
        for(int y=0;y<width;++y,data+=channels)
            memcpy(out+((width-y-1)*height+x)*channels,data,channels);
    }
}

void bitmap_rotate_180(uint8_t *data,int width,int height,int channels)
{
    const int line_size=width*channels;
    const int size=width*height*channels;
    const int top=line_size*(height-1);
    const int half=line_size/2;

    for(int offset=0;offset<size;offset+=line_size)
    {
        uint8_t *ha=data+top-offset;
        uint8_t *hb=data+offset+line_size-channels;

        for(int w=0;w<half;w+=channels)
            swap(ha+w,hb-w,channels);
    }

    if((width%2)==1)
    {
        for(int y=0;y<height/2;++y)
            swap(data+(width*y+width/2)*channels,data+(width*(height-y-1)+width/2)*channels,channels);
    }
}

void bitmap_rotate_180(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    out+=(height-1)*width*channels+(width-1)*channels;
    for(int i=0;i<width*height;++i,out-=channels,data+=channels)
        memcpy(out,data,channels);
}

void bitmap_crop(uint8_t *data,int width,int height,int x,int y,int crop_width,int crop_height,int channels)
{
    bitmap_crop(data,width,height,x,y,crop_width,crop_height,channels,data);
}

void bitmap_crop(const uint8_t *data,int width,int height,int x,int y,int crop_width,int crop_height,int channels,uint8_t *out)
{
    if(x<0)
        x=0;
    if(y<0)
        y=0;
    if(x+crop_width>width)
        crop_width=width-x;
    if(y+crop_height>height)
        crop_height=height-y;
    if(crop_width<=0 || crop_height<=0)
        return;

    data+=(width*y+x)*channels;
    const int line_size=width*channels;
    const int crop_line_size=crop_width*channels;
    for(int i=0;i<crop_height;++i,out+=crop_line_size,data+=line_size)
        memmove(out,data,crop_line_size);
}

void bitmap_resize(const uint8_t *data,int width,int height,int new_width,int new_height,int channels,uint8_t *out)
{
    const uint32_t stride=width*channels;
    const uint32_t stride_1=(width+1)*channels;
    const uint32_t x_ratio=((width-1)<<16)/new_width;
    const uint32_t y_ratio=((height-1)<<16)/new_height;
    uint64_t y=0;
    for(int i=0;i<new_height;++i)
    {
        const uint32_t yr=(uint32_t)(y>>16),y_index=yr*width;
        const uint64_t dy=y-(yr<<16),dy2=(1<<16)-dy;
        uint64_t x=0;
        for(int j=0;j<new_width;++j)
        {
            const uint32_t xr=(uint32_t)(x>>16);
            const uint64_t dx=x-(xr<<16),dx2=(1<<16)-dx;
            const uint8_t *cdata=data+(y_index+xr)*channels;
            for(int k=0;k<channels;++k,++cdata)
            {
                *out++ = (uint8_t)((cdata[0]*dx2*dy2+
                                          cdata[channels]*dx*dy2+
                                          cdata[stride]*dy*dx2+
                                          cdata[stride_1]*dx*dy)>>32);
            }
            x+=x_ratio;
        }
        y+=y_ratio;
    }
}

void bitmap_rgb_to_bgr(uint8_t *data,int width,int height,int channels)
{
    if(channels<3)
        return;

    if(channels==4 && nya_memory::is_aligned(data,4))
    {
        for(const uint8_t *to=data+width*height*4;data<to;data+=4)
            *(uint32_t*)(data) =(data[0]<<16)|(data[1]<<8)|data[2]|(data[3]<<24);
        return;
    }

    for(int i=0;i<width*height;++i,data+=channels)
    {
        const uint8_t tmp=data[0];
        data[0]=data[2];
        data[2]=tmp;
    }
}

void bitmap_rgb_to_bgr(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    if(channels==3)
    {
        for(int i=0;i<width*height;++i,data+=3,out+=3)
            out[0]=data[2],out[1]=data[1],out[2]=data[0];
    }
    else if(channels==4)
    {
        if(nya_memory::is_aligned(out,4))
        {
            uint32_t *out32=(uint32_t *)out;
            for(const uint8_t *to=data+width*height*4;data<to;data+=4)
                *out32++ =(data[0]<<16)|(data[1]<<8)|data[2]|(data[3]<<24);
            return;
        }

        for(int i=0;i<width*height;++i,data+=4,out+=4)
        {
            out[0]=data[2];
            out[1]=data[1];
            out[2]=data[0];
            out[3]=data[3];
        }
    }
    else
        memcpy(out,data,width*height*channels);
}

void bitmap_rgba_to_rgb(uint8_t *data,int width,int height)
{
    bitmap_rgba_to_rgb(data,width,height,data);
}

void bitmap_rgba_to_rgb(const uint8_t *data,int width,int height,uint8_t *out)
{
    for(const uint8_t *to=data+width*height*4;data<to;data+=4,out+=3)
    {
        out[0]=data[0];
        out[1]=data[1];
        out[2]=data[2];
    }
}

void bitmap_bgra_to_rgb(uint8_t *data,int width,int height)
{
    const uint8_t *to=data+width*height*4;
    uint8_t *out=data;

    for(int i=0;i<2 && data<to;++i,data+=4,out+=3)
    {
        const uint8_t r=data[0];
        out[0]=data[2];
        out[1]=data[1];
        out[2]=r;
    }

    for(;data<to;data+=4,out+=3)
    {
        out[0]=data[2];
        out[1]=data[1];
        out[2]=data[0];
    }
}

void bitmap_bgra_to_rgb(const uint8_t *data,int width,int height,uint8_t *out)
{
    for(const uint8_t *to=data+width*height*4;data<to;data+=4,out+=3)
    {
        out[0]=data[2];
        out[1]=data[1];
        out[2]=data[0];
    }
}

void bitmap_rgb_to_rgba(const uint8_t *data,int width,int height,uint8_t alpha,uint8_t *out)
{
    data+=width*height*3;
    out+=width*height*4;
    for(int i=0;i<width*height;++i)
    {
        out-=4,data-=3;
        out[0]=data[0];
        out[1]=data[1];
        out[2]=data[2];
        out[3]=alpha;
    }
}

void bitmap_rgb_to_bgra(const uint8_t *data,int width,int height,uint8_t alpha,uint8_t *out)
{
    data+=width*height*3;
    out+=width*height*4;
    for(int i=0;i<width*height;++i)
    {
        out-=4,data-=3;
        out[0]=data[2];
        out[1]=data[1];
        out[2]=data[0];
        out[3]=alpha;
    }
}

void bitmap_argb_to_rgba(uint8_t *data,int width,int height)
{
    bitmap_argb_to_rgba(data,width,height,data);
}

void bitmap_argb_to_rgba(const uint8_t *data,int width,int height,uint8_t *out)
{
    const uint32_t *data32=(uint32_t *)data;
    uint32_t *out32=(uint32_t *)out;
    const uint32_t *to=data32+width*height;
    while(data32<to)
    {
        const uint32_t c= *data32++;
        *out32++ =c<<24|c>>8;
    }
}

void bitmap_argb_to_bgra(uint8_t *data,int width,int height)
{
    bitmap_argb_to_bgra(data,width,height,data);
}

void bitmap_argb_to_bgra(const uint8_t *data,int width,int height,uint8_t *out)
{
    const uint32_t *data32=(uint32_t *)data;
    uint32_t *out32=(uint32_t *)out;
    const uint32_t *to=data32+width*height;
    while(data32<to)
    {
        const uint32_t c= *data32++;
        *out32++ =c<<24|(c<<8 & 0x00FF0000)|(c>>8 & 0x0000FF00)|c>>24;
    }
}

inline void color_to_yuv420(const uint8_t *data[3],int width,int height,int channels,uint8_t *out)
{
    int image_size=width*height;
    uint8_t *dst_y=out;
    uint8_t *dst_u=dst_y+image_size;
    uint8_t *dst_v=dst_u+image_size/4;
    image_size*=channels;

    for(int i=0;i<image_size;i+=channels)
        *dst_y++ =((66*data[0][i] + 129*data[1][i] + 25*data[2][i])>>8)+16;

    const int stride=width*channels;
    const int channels2=channels*2;
    for(int y=0,i=0;y<height;y+=2,i+=stride)
    {
        for(int x=0;x<width;x+=2,i+=channels2)
        {
            *dst_u++ =((-38*data[0][i] - 74*data[1][i] + 112*data[2][i])>>8)+128;
            *dst_v++ =((112*data[0][i] - 94*data[1][i] -  18*data[2][i])>>8)+128;
        }
    }
}

void bitmap_rgb_to_yuv420(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    const uint8_t *rgb[]={data,data+1,data+2};
    if(channels==1)
        rgb[2]=rgb[1]=rgb[0];
    color_to_yuv420(rgb,width,height,channels,out);
}

void bitmap_bgr_to_yuv420(const uint8_t *data,int width,int height,int channels,uint8_t *out)
{
    const uint8_t *bgr[]={data+2,data+1,data};
    if(channels==1)
        bgr[0]=bgr[1]=bgr[2];
    color_to_yuv420(bgr,width,height,channels,out);
}

inline uint8_t clamp(int c) { return c<0?0:(c>255?255:c); }

void bitmap_yuv420_to_rgb(const uint8_t *data,int width,int height,uint8_t *out)
{
    const size_t image_size=width*height;
    const uint8_t *udata=data+image_size;
    const uint8_t *vdata=udata+image_size/4;
    const int half_width=width/2;

    for(int y=0;y<height;++y)
    {
        const int hidx=y/2*half_width;
        for(int x=0;x<width;++x)
        {
            const int y0=(int)*data++;
            const int idx=hidx+x/2;
            const int u0=(int)udata[idx]-128;
            const int v0=(int)vdata[idx]-128;

            const int y1=1192*(y0>16?y0-16:0);
            const int u1=400*u0, u2=2066*u0;
            const int v1=1634*v0, v2=832*v0;

            *out++ =clamp((y1+v1)>>10);
            *out++ =clamp((y1-v2-u1)>>10);
            *out++ =clamp((y1+u2)>>10);
        }
    }
}
    
bool bitmap_is_full_alpha(const uint8_t *data,int width,int height)
{
    data+=3;
    const uint8_t *to=data+width*height*4;
    while(data<to)
    {
        if(*data!=0xff)
            return false;
        data+=4;
    }
    return true;
}

}
