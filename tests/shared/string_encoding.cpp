//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "string_encoding.h"
#include "moonspeak.h"
#include <string.h>
#include <vector>

std::string utf8_from_utf16le(const void *data,unsigned int size)
{
    if(!data || !size)
        return std::string();

    std::string result;

    const unsigned short* src=(unsigned short*)data;
    const unsigned short* end=src+size/2;
    while(src<end)
    {
        unsigned int c= *src++;
        const unsigned int hstart=0xD800;
        if(c>=hstart && c<=0xDBFF)
        {
            if(src>=end)
                return result;
            
            const unsigned int c2= *src;
            const unsigned int lstart=0xDC00;
            if(c2>=lstart && c2<=0xDFFF)
            {
                c=((c-hstart)<<10)+(c2-lstart)+0x10000;
                ++src;
            }
        }
        
        int count=3;
        if(c<0x80) count=1;
        else if(c<0x800) count=2;
        else if(c<0x10000) count=3;
        else if(c<0x110000) count=4;
        else c=0x0000FFFD;

        result.resize(result.size()+count);

        for(int j=1;j<count;++j)
        {
            result[result.size()-j]=char((c|0x80) & 0xBF);
            c>>=6;
        }

        const unsigned char marks[5]={0x00,0x00,0xC0,0xE0,0xF0};
        result[result.size()-count]=char(c|marks[count]);
    }

    result.resize(strlen(result.c_str()));

    return result;
}

std::string utf8_from_shiftjis(const void *data,unsigned int size)
{
    if (!data || !size)
        return std::string();

    typedef unsigned short ushort;
    std::vector<ushort> utf16;

    const unsigned char *src=(unsigned char*)data;
    for(unsigned int i=0;i<size;++i)
    {
        const ushort c=src[i];
        if(c<=0x7f || (0xa1<=c && c<=0xdf))
        {
            const ushort w=cp932_table(c);
            utf16.push_back((c && !w)?0xff1f:w);
            continue;
        }

        const ushort c2=(++i<size)?src[i]:0;
        if (((0x81<=c && c<=0x9f) || (0xe0<=c && c<=0xfc)) &&
            ((0x40<=c2 && c2<=0x7e) || (0x80<=c2 && c2<=0xfc)))
        {
            const ushort w=cp932_table(((c<<8) & 0xff00) | (c2 & 0x00ff));
            utf16.push_back(w?w:0xff1f);
            continue;
        }

        break;
    }

    return std::string(utf8_from_utf16le(&utf16[0],(unsigned int)(utf16.size()*2)));
}
