//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>
#include <stddef.h>

namespace nya_formats
{

struct tga
{
    enum color_mode
    {
        greyscale=1,
        bgr=3,
        bgra=4
    };

    int width;
    int height;
    color_mode channels;
    bool rle;
    bool horisontal_flip;
    bool vertical_flip;

    const void *data;
    size_t compressed_size;
    size_t uncompressed_size;

public:
    size_t decode_header(const void *data,size_t size); //0 if invalid
    bool decode_rle(void *decoded_data) const; //decoded_data must be allocated with uncompressed_size
    void flip_horisontal(const void *from_data,void *to_data); //to_data must be allocated, to_data could be equal to from_data
    void flip_vertical(const void *from_data,void *to_data);

public:
    size_t encode_header(void *to_data,size_t to_size=tga_header_size);
    size_t encode_rle(void *to_data,size_t to_size); //to_data size should be allocated with enough size

public:
    const static size_t tga_header_size=18;

    const static size_t tga_minimum_header_size=tga_header_size; //ToDo: remove legacy

public:
    tga(): width(0),height(0),rle(false),horisontal_flip(false),vertical_flip(false),data(0),compressed_size(0),uncompressed_size(0) {}
};

class tga_file
{
public:
    bool load(const char *file_name);
    bool create(int width,int height,tga::color_mode channels,const void *data);
    bool decode_rle();
    bool encode_rle(size_t max_compressed_size);
    bool save(const char *file_name);
    bool flip_horisontal();
    bool flip_vertical();
    void release() { m_header=nya_formats::tga(); m_data.clear(); }

public:
    bool is_rle() const { return m_header.rle; }
    bool is_flipped_vertical() const { return m_header.vertical_flip; }
    bool is_flipped_horisontal() const { return m_header.horisontal_flip; }
    int get_width() const { return m_header.width; }
    int get_height() const { return m_header.height; }
    tga::color_mode get_channels() const { return m_header.channels; }
    const unsigned char *get_data() const { if(m_data.empty()) return 0; return &m_data[0]; }
    unsigned char *get_data() { if(m_data.empty()) return 0; return &m_data[0]; }
    size_t get_data_size() { return m_data.size(); }

private:
    tga m_header;
    std::vector<unsigned char> m_data;
};

}
