//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <stdint.h>

namespace nya_render
{

void bitmap_downsample2x(uint8_t *data,int width,int height,int channels);
void bitmap_downsample2x(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmap_flip_vertical(uint8_t *data,int width,int height,int channels);
void bitmap_flip_vertical(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmap_flip_horisontal(uint8_t *data,int width,int height,int channels);
void bitmap_flip_horisontal(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmap_rotate_90_left(uint8_t *data,int width,int height,int channels);
void bitmap_rotate_90_left(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmap_rotate_90_right(uint8_t *data,int width,int height,int channels);
void bitmap_rotate_90_right(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmap_rotate_180(uint8_t *data,int width,int height,int channels);
void bitmap_rotate_180(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmap_crop(uint8_t *data,int width,int height,int x,int y,int crop_width,int crop_height,int channels);
void bitmap_crop(const uint8_t *data,int width,int height,int x,int y,int crop_width,int crop_height,int channels,uint8_t *out);

void bitmap_resize(const uint8_t *data,int width,int height,int new_width,int new_height,int channels,uint8_t *out);

void bitmap_rgb_to_bgr(uint8_t *data,int width,int height,int channels);
void bitmap_rgb_to_bgr(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmap_rgba_to_rgb(uint8_t *data,int width,int height);
void bitmap_rgba_to_rgb(const uint8_t *data,int width,int height,uint8_t *out);
void bitmap_bgra_to_rgb(uint8_t *data,int width,int height);
void bitmap_bgra_to_rgb(const uint8_t *data,int width,int height,uint8_t *out);

void bitmap_rgb_to_rgba(const uint8_t *data,int width,int height,uint8_t alpha,uint8_t *out);
void bitmap_rgb_to_bgra(const uint8_t *data,int width,int height,uint8_t alpha,uint8_t *out);

void bitmap_argb_to_rgba(uint8_t *data,int width,int height);
void bitmap_argb_to_rgba(const uint8_t *data,int width,int height,uint8_t *out);
void bitmap_argb_to_bgra(uint8_t *data,int width,int height);
void bitmap_argb_to_bgra(const uint8_t *data,int width,int height,uint8_t *out);

void bitmap_rgb_to_yuv420(const uint8_t *data,int width,int height,int channels,uint8_t *out);
void bitmap_bgr_to_yuv420(const uint8_t *data,int width,int height,int channels,uint8_t *out);
void bitmap_yuv420_to_rgb(const uint8_t *data,int width,int height,uint8_t *out);

bool bitmap_is_full_alpha(const uint8_t *data,int width,int height);

}
