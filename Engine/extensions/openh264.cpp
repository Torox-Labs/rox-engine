//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

//requires: libopenh264 http://www.openh264.org/

#include "openh264.h"
#include "codec_api.h"
#include "render/bitmap.h"
#include "memory/memory_reader.h"
#include "memory/tmp_buffer.h"
#include <ctime>

bool video_encoder::open(const char *output_file,int width,int height,float fps,container_type c,int bitrate)
{
    if(m_encoder)
        close();

    if(!output_file)
        return false;

    if(WelsCreateSVCEncoder(&m_encoder)!=0 || !m_encoder)
        return false;

    SEncParamBase param;
    memset(&param,0,sizeof(SEncParamBase));
    param.iUsageType=CAMERA_VIDEO_REAL_TIME;
    param.fMaxFrameRate=fps;
    param.iPicWidth=width;
    param.iPicHeight=height;
    param.iTargetBitrate=bitrate*1000;
    if(m_encoder->Initialize(&param)!=0)
    {
        WelsDestroySVCEncoder(m_encoder);
        m_encoder=0;
        return false;
    }

    //encoder->SetOption(ENCODER_OPTION_TRACE_LEVEL,&trace_level);
    int videoFormat=videoFormatI420;
    if(m_encoder->SetOption(ENCODER_OPTION_DATAFORMAT,&videoFormat)!=cmResultSuccess)
    {
        close();
        return false;
    }

    m_out=fopen(output_file,"wb");
    if(!m_out)
    {
        close();
        return false;
    }

    m_width=width;
    m_height=height;
    m_fps=fps;
    m_duration=0;
    m_container=c;

    if(m_container==container_h264)
        return true;

    chunk ftyp("ftyp");
    ftyp.append_buf("mp42",4);
    ftyp.append_uint(0);
    ftyp.append_buf("isommp42",8);
    write_chunk(ftyp);

    m_mdat_offset=ftell(m_out);
    write_header("mdat",0);

    return true;
}

inline void change_endianness4(uint32_t &v)
{
    v=(v<<24) | ((v<<8) & 0x00ff0000) | ((v>>8) & 0x0000ff00) | ((v>>24) & 0x000000ff);
}

inline void change_endianness2(uint16_t &v)
{
    v=((v & 0x00ff)<<8) | ((v & 0xff00)>>8);
}

template<typename t> void add_unique_ps(const t &o,std::list<t> &list)
{
    for(typename std::list<t>::iterator it=list.begin();it!=list.end();++it) if(*it==o) return;
    list.push_back(o);
}

bool video_encoder::add_frame(const void *buf,color_mode c)
{
    if(!buf || !m_encoder)
        return false;

    nya_memory::tmp_buffer_ref tbuf;

    if(c!=color_yuv420)
    {
        tbuf.allocate(m_width*m_height+(m_width*m_height)/2);
        const unsigned char *from=(unsigned char *)buf;
        unsigned char *to=(unsigned char *)tbuf.get_data();

        switch(c)
        {
            case color_yuv420: break;
            case color_rgb: nya_render::bitmap_rgb_to_yuv420(from,m_width,m_height,3,to); break;
            case color_rgba: nya_render::bitmap_rgb_to_yuv420(from,m_width,m_height,4,to); break;
            case color_bgr: nya_render::bitmap_bgr_to_yuv420(from,m_width,m_height,3,to); break;
            case color_bgra: nya_render::bitmap_bgr_to_yuv420(from,m_width,m_height,4,to); break;
        }

        buf=to;
    }

    SFrameBSInfo info;
    memset(&info,0,sizeof(SFrameBSInfo));
    SSourcePicture pic;
    memset(&pic,0,sizeof(SSourcePicture));
    pic.iPicWidth=m_width;
    pic.iPicHeight=m_height;
    pic.iColorFormat=videoFormatI420;
    pic.iStride[0]=m_width;
    pic.iStride[1]=pic.iStride[2]=m_width/2;
    pic.pData[0]=(unsigned char *)buf;
    pic.pData[1]=pic.pData[0]+m_width*m_height;
    pic.pData[2]=pic.pData[1]+(m_width*m_height)/4;

    const int rv=m_encoder->EncodeFrame(&pic, &info);
    tbuf.free();
    if(rv!=cmResultSuccess)
        return false;

    if(info.eFrameType==videoFrameTypeInvalid)
        return false;

    if(m_container==container_h264)
    {
        for(int i=0;i<info.iLayerNum;++i)
        {
            unsigned char *buf=info.sLayerInfo[i].pBsBuf;
            size_t size=0;
            for(int j=0;j<info.sLayerInfo[i].iNalCount;++j)
                size+=info.sLayerInfo[i].pNalLengthInByte[j];

            write_buf(buf, size);
        }

        return true;
    }

    for(int i=0;i<info.iLayerNum;++i)
    {
        unsigned char *buf=info.sLayerInfo[i].pBsBuf;
        for(int j=0;j<info.sLayerInfo[i].iNalCount;++j)
        {
            int nal_size=info.sLayerInfo[i].pNalLengthInByte[j];
            if(nal_size<4)
                continue;

            const int nal_type=buf[buf[3]==1?4:5] & 0x1f;
            if(buf[3]==0 && buf[4]==1)
                ++buf,--nal_size;

            if(nal_type==7 || nal_type==8)
            {
                nal_size-=5,buf+=5;
                std::vector<char> ps(nal_size);
                memcpy(ps.data(),buf,nal_size);
                if(nal_type==7)
                    add_unique_ps(ps,m_sps_list);
                else if(nal_type==8)
                    add_unique_ps(ps,m_pps_list);
            }
            else
            {
                uint32_t size=nal_size-4;
                change_endianness4(size);
                memcpy(buf,&size,sizeof(size));

                write_buf(buf,nal_size);
                m_frame_sizes.push_back(nal_size);
            }

            buf+=nal_size;
        }
    }

    ++m_duration;
    return true;
}

bool video_encoder::close()
{
    if(m_encoder)
    {
        m_encoder->Uninitialize();
        WelsDestroySVCEncoder(m_encoder);
        m_encoder=0;
    }

    if(!m_out)
        return false;

    if(m_container==container_h264)
    {
        fclose(m_out);
        return true;
    }

    const size_t mdat_end=ftell(m_out);
    fseek(m_out,m_mdat_offset,SEEK_SET);
    write_header("mdat",uint32_t(mdat_end-m_mdat_offset));
    fseek(m_out,mdat_end,SEEK_SET);

    if(m_sps_list.empty())
    {
        fclose(m_out);
        return false;
    }

    const uint32_t unix_to_apl=(66*365+17) * 60*60*24;

    const uint32_t create_time=(uint32_t)time(0)+unix_to_apl;
    const uint32_t modif_time=create_time;
    const uint8_t profile=m_sps_list.front()[0];
    const uint8_t comp_profile=m_sps_list.front()[1];
    const uint8_t level=m_sps_list.front()[2];
    const uint32_t timescale=1000;
    const uint32_t duration=m_duration*timescale/m_fps;
    const uint32_t rate=65536;
    const uint16_t volume=256;
    const uint16_t resolution=72;
    const uint16_t depth=24;
    const uint32_t matrix[]={256,0,0,0,256,0,0,0,64};

    chunk moov("moov");

    chunk mvhd("mvhd");
    mvhd.append_uint(0); //version
    mvhd.append_uint(create_time);
    mvhd.append_uint(modif_time);
    mvhd.append_uint(timescale);
    mvhd.append_uint(duration);
    mvhd.append_uint(rate);
    mvhd.append_ushort(volume);
    mvhd.append_byte(0,10); //reserved
    mvhd.append_buf(matrix,sizeof(matrix));
    mvhd.append_byte(0,24); //predefined
    mvhd.append_uint(2); //mext track
    moov.append(mvhd);

    chunk trak("trak");

    chunk tkhd("tkhd");
    tkhd.append_uint(3); //version, flags
    tkhd.append_uint(create_time);
    tkhd.append_uint(modif_time);
    tkhd.append_uint(1); //track id
    tkhd.append_uint(0);
    tkhd.append_uint(duration);
    tkhd.append_byte(0,16);
    tkhd.append_buf(matrix,sizeof(matrix));
    tkhd.append_ushort(m_width);
    tkhd.append_ushort(0);
    tkhd.append_ushort(m_height);
    tkhd.append_ushort(0);
    trak.append(tkhd);

    chunk edts("edts");
    chunk elst("elst");
    elst.append_uint(0); //version
    elst.append_uint(1); //count
    elst.append_uint(duration);
    elst.append_uint(0);
    elst.append_uint(rate);
    edts.append(elst);
    trak.append(edts);

    chunk mdia("mdia");

    chunk mdhd("mdhd");
    mdhd.append_uint(0); //version
    mdhd.append_uint(create_time);
    mdhd.append_uint(modif_time);
    mdhd.append_uint(timescale);
    mdhd.append_uint(duration);
    mdhd.append_uint(0); //lang
    mdia.append(mdhd);

    chunk hdlr("hdlr");
    hdlr.append_uint(0); //version
    hdlr.append_uint(0); //predefined
    hdlr.append_buf("vide",4);
    hdlr.append_byte(0,12); //reserved
    const char hdlr_name[]="Video";
    hdlr.append_buf(hdlr_name,sizeof(hdlr_name));
    mdia.append(hdlr);

    chunk minf("minf");

    chunk vmhd("vmhd");
    vmhd.append_uint(1); //version, flags
    vmhd.append_byte(0,8);
    minf.append(vmhd);

    chunk dinf("dinf");
    chunk dref("dref");
    dref.append_uint(0); //version
    dref.append_uint(1); //count
    chunk url("url ");
    url.append_uint(1);
    dref.append(url);
    dinf.append(dref);
    minf.append(dinf);

    chunk stbl("stbl");
    chunk stsd("stsd");
    stsd.append_uint(0);
    stsd.append_uint(1);
    chunk avc1("avc1");
    avc1.append_uint(0);
    avc1.append_uint(1);
    avc1.append_byte(0,16);
    avc1.append_ushort(m_width);
    avc1.append_ushort(m_height);
    avc1.append_ushort(resolution);
    avc1.append_ushort(0);
    avc1.append_ushort(resolution);
    avc1.append_ushort(0);
    avc1.append_uint(0);
    avc1.append_ushort(1);
    const char compressor_name[31]="OpenH264";
    avc1.append_byte(strlen(compressor_name));
    avc1.append_buf(compressor_name,sizeof(compressor_name));
    avc1.append_ushort(depth);
    avc1.append_ushort(-1);

    chunk avcc("avcC");
    avcc.append_byte(1); //version
    avcc.append_byte(profile);
    avcc.append_byte(comp_profile);
    avcc.append_byte(level);
    avcc.append_byte(-1);
    avcc.append_byte(uint8_t(m_sps_list.size()) | 0xe0);
    for(ps_list::iterator it=m_sps_list.begin();it!=m_sps_list.end();++it)
    {
        avcc.append_ushort(it->size()+1);
        avcc.append_byte(103);
        avcc.append_buf(it->data(),it->size());
    }
    avcc.append_byte(uint8_t(m_pps_list.size()));
    for(ps_list::iterator it=m_pps_list.begin();it!=m_pps_list.end();++it)
    {
        avcc.append_ushort(it->size()+1);
        avcc.append_byte(104);
        avcc.append_buf(it->data(),it->size());
    }
    avc1.append(avcc);
    stsd.append(avc1);
    stbl.children.push_back(stsd);

    chunk stts("stts");
    stts.append_uint(0); //version
    stts.append_uint(1); //count
    stts.append_uint(m_duration);
    stts.append_uint(duration/m_duration);
    stbl.append(stts);

    chunk stss("stss");
    stss.append_uint(0); //version
    stss.append_uint(1); //count
    stss.append_uint(1); //sample num
    stbl.append(stss);

    chunk stsc("stsc");
    stsc.append_uint(0); //version
    stsc.append_uint(1); //count
    stsc.append_uint(1); //first
    stsc.append_uint(uint32_t(m_frame_sizes.size()));
    stsc.append_uint(1); //desc
    stbl.append(stsc);

    chunk stsz("stsz");
    stsz.append_uint(0); //version
    stsz.append_uint(0); //sample size
    stsz.append_uint(uint32_t(m_frame_sizes.size()));
    for(size_t i=0;i<m_frame_sizes.size();++i)
        stsz.append_uint(uint32_t(m_frame_sizes[i]));
    stbl.append(stsz);

    chunk stco("stco");
    stco.append_uint(0); //version
    stco.append_uint(1); //count
    stco.append_uint((uint32_t)m_mdat_offset+8);
    stbl.append(stco);

    minf.append(stbl);
    mdia.append(minf);
    trak.append(mdia);
    moov.append(trak);

    write_chunk(moov);
    fclose(m_out);
    return true;
}

void video_encoder::write_header(const char *name,size_t size)
{
    struct { uint32_t size,name; } header;
    header.size=(uint32_t)size;
    change_endianness4(header.size);
    memcpy(&header.name,name,sizeof(uint32_t));
    write_buf(&header,sizeof(header));
}

void video_encoder::write_buf(const void *data,size_t size)
{
    fwrite(data,1,size,m_out);
}

void video_encoder::write_chunk(const chunk &c)
{
    uint32_t size=(uint32_t)c.get_size();
    write_header(c.id.c_str(),size);
    if(!c.data.empty())
        write_buf(&c.data[0],c.data.size());
    for(std::list<chunk>::const_iterator it=c.children.begin();it!=c.children.end();++it)
        write_chunk(*it);
}

void video_encoder::chunk::append_uint(unsigned int u)
{
    change_endianness4(u);
    const size_t off=data.size();
    data.resize(off+sizeof(u));
    memcpy(&data[off],&u,sizeof(u));
}

void video_encoder::chunk::append_ushort(unsigned short u)
{
    change_endianness2(u);
    const size_t off=data.size();
    data.resize(off+sizeof(u));
    memcpy(&data[off],&u,sizeof(u));
}

void video_encoder::chunk::append_byte(unsigned char u) { data.push_back(u); }
void video_encoder::chunk::append_byte(unsigned char u,int count) { data.resize(data.size()+count,u); }

void video_encoder::chunk::append_buf(const void *buf,size_t size)
{
    const size_t off=data.size();
    data.resize(off+size);
    memcpy(&data[off],buf,size);
}

namespace
{

struct atom
{
    const char *data;
    uint32_t size;
    uint32_t id;
    std::vector<atom> childs;

    atom(): data(0),size(0) {}

    bool is_valid() { return size>0; }

    bool parse_header(nya_resources::resource_data *data,size_t offset)
    {
        struct { uint32_t size,type; } size_type;

        if(!data->read_chunk(&size_type,sizeof(size_type),offset))
            return false;

        change_endianness4(size_type.size);

        if(size_type.size==1)
            return false; //ToDo: read uint64 size

        if(size_type.size<sizeof(size_type))
            return false;

        if(offset+size_type.size>data->get_size())
            return false;

        id=size_type.type;
        size=size_type.size;
        return true;
    }

    bool is(const char *name) { return memcmp(name,&id,4)==0; }

    int child_count()
    {
        if(childs.empty())
        {
            nya_memory::memory_reader r(data,size);
            while(r.get_remained()>0)
            {
                atom a;
                a.size=r.read<uint32_t>();
                change_endianness4(a.size);
                a.id=r.read<uint32_t>();
                //ToDo: size==1
                if(a.size<8 || a.size-8>r.get_remained())
                {
                    childs.clear();
                    return 0;
                }
                a.data=(char *)r.get_data();
                a.size-=8;
                r.skip(a.size);
                childs.push_back(a);
            }
        }

        return (int)childs.size();
    }

    atom child(int idx) { return childs[idx]; }

    atom child(const char *name)
    {
        for(int i=0;i<child_count();++i)
        {
            if(childs[i].is(name))
                return childs[i];
        }

        return atom();
    }

    bool skip(uint32_t count)
    {
        if(count>size)
            return false;

        size-=count;
        data+=count;
        return true;
    }

    uint8_t read_uint8() { return read<uint8_t>(); }
    uint16_t read_uint16() { uint16_t v=read<uint16_t>(); change_endianness2(v); return v; }
    uint32_t read_uint32() { uint32_t v=read<uint32_t>(); change_endianness4(v); return v; }

private:
    template<typename t> t read()
    {
        if(size<sizeof(t))
            return t();

        t v;
        memcpy(&v,data,sizeof(t));
        size-=sizeof(t);
        data+=sizeof(t);
        return v;
    }
};

}

bool video_decoder::open(const char *name)
{
    return open(nya_resources::get_resources_provider().access(name));
}

bool video_decoder::open(nya_resources::resource_data *data)
{
    if(!data)
        return false;

    m_frame_duration=0;
    m_width=0,m_height=0;
    m_offset=0;
    m_current_frame=0;

    typedef std::list<std::vector<unsigned char> > nals_list;
    nals_list init_nals;

    atom a;
    for(size_t offset=0,size=0;offset<data->get_size();offset+=size)
    {
        if(!a.parse_header(data,offset))
            break;

        size=a.size;

        if(!a.is("moov"))
            continue;

        nya_memory::tmp_buffer_scoped buf(a.size);
        if(!data->read_chunk(buf.get_data(),buf.get_size(),offset))
            break;

        a.data=(char *)buf.get_data(8);
        a.size=(uint32_t)buf.get_size()-8;
        for(int i=0;i<a.child_count();++i)
        {
            atom track=a.child(i);
            if(!track.is("trak"))
                continue;

            atom mdia=track.child("mdia");
            atom stbl=mdia.child("minf").child("stbl");

            atom stsd=stbl.child("stsd");
            stsd.skip(8);
            atom avc1=stsd.child("avc1");
            if(!avc1.is_valid())
                continue;

            atom mdhd=mdia.child("mdhd");
            mdhd.skip(12);
            const uint32_t timescale=mdhd.read_uint32();
            if(!timescale)
                break;

            atom stts=stbl.child("stts");
            stts.skip(12);
            uint32_t frame_duration=stts.read_uint32();

            avc1.skip(24);
            m_width=avc1.read_uint16();
            m_height=avc1.read_uint16();
            m_frame_duration=int(frame_duration*1000/timescale);
            avc1.skip(50);
            atom avcc=avc1.child("avcC");
            avcc.skip(5);
            for(int i=0;i<2;++i)
            {
                uint8_t count=avcc.read_uint8();
                if(i==0) count &=0x1f;
                for(int j=0;j<count;++j)
                {
                    uint16_t size=avcc.read_uint16();
                    nals_list::value_type data(4+size,0);
                    data[3]=1; //header
                    memcpy(&data[4],avcc.data,data.size()-4);
                    avcc.skip(size);
                    init_nals.push_back(data);
                }
            }

            atom stco=stbl.child("stco");
            stco.skip(8);
            m_offset=stco.read_uint32(); //ToDo: multiple chunks

            atom stsz=stbl.child("stsz");
            stsz.skip(8);
            m_frame_sizes.resize(stsz.read_uint32());
            for(int i=0;i<(int)m_frame_sizes.size();++i)
                m_frame_sizes[i]=stsz.read_uint32();

            break;
        }
    }

    if(!m_frame_duration || !m_width || !m_height)
    {
        data->release();
        return false;  //ToDo: parse .h264
    }

    m_data=data;

    if(WelsCreateDecoder(&m_decoder)!=0 || !m_decoder)
    {
        m_decoder=0;
        close();
        return false;
    }

    SDecodingParam param;
    memset(&param,0,sizeof(SDecodingParam));
    param.sVideoProperty.eVideoBsType=VIDEO_BITSTREAM_AVC;
    if(m_decoder->Initialize(&param)!=0)
    {
        close();
        return false;
    }

    for(nals_list::const_iterator it=init_nals.begin();it!=init_nals.end();++it)
    {
        SBufferInfo info;
        memset(&info,0,sizeof(info));
        unsigned char *dst[3];

        auto status=m_decoder->DecodeFrame2(it->data(),(int)it->size(),dst,&info);
        if(status!=dsErrorFree)
        {
            close();
            return false;
        }

        if(info.iBufferStatus==0)
            m_decoder->DecodeFrame2(NULL,0,dst,&info);
    }

    return true;
}

size_t video_decoder::get_frame_size_yuv420() const
{
    if(!m_decoder)
        return 0;

    return m_width*m_height + m_width*m_height/2;
}

bool video_decoder::decode_frame_yuv420(void *data)
{
    if(!m_decoder || !data)
        return false;

    if(m_current_frame>=(int)m_frame_sizes.size())
        return false;

    uint32_t size=m_frame_sizes[m_current_frame];
    nya_memory::tmp_buffer_scoped frame_buf(size);
    if(!m_data->read_chunk(frame_buf.get_data(),size,m_offset))
        return false;

    m_offset+=size;
    ++m_current_frame;

    const char nal_header[]={0,0,0,1};
    memcpy(frame_buf.get_data(),nal_header,sizeof(nal_header));

    SBufferInfo info;
    memset(&info,0,sizeof(info));
    unsigned char* color_data[3];

    if(m_decoder->DecodeFrame2((unsigned char *)frame_buf.get_data(),(int)frame_buf.get_size(),color_data,&info)!=dsErrorFree)
        return false;

    if(info.iBufferStatus==0)
        m_decoder->DecodeFrame2(NULL,0,color_data,&info);

    if(info.iBufferStatus!=1)
        return false;

    if(info.UsrData.sSystemBuffer.iWidth!=m_width || info.UsrData.sSystemBuffer.iHeight!=m_height)
        return false;

    for(int i=0;i<3;++i)
    {
        const int width=i<1?m_width:m_width/2;
        const int height=i<1?m_height:m_height/2;
        const int stride=info.UsrData.sSystemBuffer.iStride[i<1?0:1];
        for(int j=0;j<height;++j)
        {
            memcpy(data,color_data[i]+stride*(height-j-1),width);
            data=(char *)data+width;
        }
    }

    return true;
}

size_t video_decoder::get_frame_size_rgb() const
{
    if(!m_decoder)
        return 0;

    return m_width*m_height*3;
}

bool video_decoder::decode_frame_rgb(void *data)
{
    nya_memory::tmp_buffer_scoped buf(get_frame_size_yuv420());
    if(!decode_frame_yuv420(buf.get_data()))
        return false;

    nya_render::bitmap_yuv420_to_rgb((unsigned char *)buf.get_data(),m_width,m_height,(unsigned char *)data);
    return true;
}

void video_decoder::close()
{
    if(m_data)
        m_data->release();
    m_data=0;

    if(m_decoder)
        WelsDestroyDecoder(m_decoder);
    m_decoder=0;
}
