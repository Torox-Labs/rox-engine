//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <list>
#include <vector>
#include "resources/resources.h"

class ISVCEncoder;

class video_encoder
{
public:
    enum container_type
    {
        container_h264,
        container_mp4
    };

    bool open(const char *output_file,int width,int height,float fps,container_type c=container_mp4,int bitrate=5000);

    enum color_mode
    {
        color_yuv420,
        color_rgb,
        color_bgr,
        color_rgba,
        color_bgra,
    };

    bool add_frame(const void *buf,color_mode c);
    bool close();

public:
    int get_width() const { return m_encoder?m_width:0; }
    int get_height() const { return m_encoder?m_height:0; }
    int get_fps() const { return m_encoder?m_fps:0; }
    int get_frames_count() const { return m_encoder?m_duration:0; }
    int get_duration() const { return get_fps()>0?m_duration*1000/m_fps:0; }

public:
    video_encoder(): m_encoder(0),m_out(0) {}

private:
    void write_buf(const void *data,size_t size);
    void write_header(const char *name,size_t size);

    struct chunk
    {
        std::string id;
        std::vector<char> data;
        std::list<chunk> children;

        size_t get_size() const
        {
            size_t size=data.size()+8;
            for(std::list<chunk>::const_iterator it=children.begin();it!=children.end();++it)
                size+=it->get_size();
            return size;
        }

        void append_uint(unsigned int u);
        void append_ushort(unsigned short u);
        void append_byte(unsigned char u);
        void append_byte(unsigned char u,int count);
        void append_buf(const void *buf,size_t size);

        void append(chunk &c) { children.push_back(c); }

        chunk() {}
        chunk(std::string id): id(id) {}
    };

    void write_chunk(const chunk &c);

private:
    int m_width;
    int m_height;
    int m_fps;
    int m_duration;
    container_type m_container;

    ISVCEncoder *m_encoder;
    FILE *m_out;
    size_t m_mdat_offset;

    typedef std::list<std::vector<char> > ps_list;
    ps_list m_sps_list;
    ps_list m_pps_list;
    std::vector<size_t> m_frame_sizes;
};

class ISVCDecoder;

class video_decoder
{
public:
    bool open(const char *name);
    bool open(nya_resources::resource_data *data);
    void close();

public:
    size_t get_frame_size_yuv420() const;
    bool decode_frame_yuv420(void *data);

    size_t get_frame_size_rgb() const;
    bool decode_frame_rgb(void *data);

public:
    int get_width() const { return m_decoder?m_width:0; }
    int get_height() const { return m_decoder?m_height:0; }
    int get_duration() const { return m_decoder?(int)m_frame_sizes.size()*m_frame_duration:0; }
    int get_frame_duration() const { return m_decoder?m_frame_duration:0; }
    float get_fps() const { return m_decoder && m_frame_duration?1000.0f/m_frame_duration:0.0f; }

public:
    video_decoder(): m_decoder(0),m_data(0) {}

private:
    bool parse_chunks(std::string parent,size_t offset);

private:
    int m_width;
    int m_height;
    int m_frame_duration;
    size_t m_offset;
    std::vector<unsigned int> m_frame_sizes;
    int m_current_frame;
    ISVCDecoder *m_decoder;
    nya_resources::resource_data *m_data;
};
