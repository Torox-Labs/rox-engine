// Updated By the ROX_ENGINE
// Copyright (C) 2024 Torox Project
// Portions Copyright (C) 2013 nyan.developer@gmail.com (nya-engine)
//
// This file was modified by the Torox Project.
// 
// This file incorporates code from the nya-engine project, which is licensed under the MIT License.
// See the LICENSE-MIT file in the root directory for more information.
//
// This file is also part of the Rox-engine, which is licensed under a dual-license system:
// 1. Free Use License (for non-commercial and commercial use under specific conditions)
// 2. Commercial License (for use on proprietary platforms)
// See the LICENSE file in the root directory for the full Rox-engine license terms.

#pragma once

#include <stdint.h>

namespace RoxRender
{

void bitmapDownsample2x(uint8_t *data,int width,int height,int channels);
void bitmapDownsample2x(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmapFlipVertical(uint8_t *data,int width,int height,int channels);
void bitmapFlipVertical(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmapFlipHorisontal(uint8_t *data,int width,int height,int channels);
void bitmapFlipHorisontal(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmapRotate_90_left(uint8_t *data,int width,int height,int channels);
void bitmapRotate_90_left(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmapRotate_90_right(uint8_t *data,int width,int height,int channels);
void bitmapRotate_90_right(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmapRotate_180(uint8_t *data,int width,int height,int channels);
void bitmapRotate_180(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmapCrop(uint8_t *data,int width,int height,int x,int y,int crop_width,int crop_height,int channels);
void bitmapCrop(const uint8_t *data,int width,int height,int x,int y,int crop_width,int crop_height,int channels,uint8_t *out);

void bitmapResize(const uint8_t *data,int width,int height,int new_width,int new_height,int channels,uint8_t *out);

void bitmapRgbToBgr(uint8_t *data,int width,int height,int channels);
void bitmapRgbToBgr(const uint8_t *data,int width,int height,int channels,uint8_t *out);

void bitmapRgbaToRgb(uint8_t *data,int width,int height);
void bitmapRgbaToRgb(const uint8_t *data,int width,int height,uint8_t *out);
void bitmapRgraToRgb(uint8_t *data,int width,int height);
void bitmapRgraToRgb(const uint8_t *data,int width,int height,uint8_t *out);

void bitmapRgbToRgba(const uint8_t *data,int width,int height,uint8_t alpha,uint8_t *out);
void bitmapRgbToBgra(const uint8_t *data,int width,int height,uint8_t alpha,uint8_t *out);

void bitmapArgbToRgba(uint8_t *data,int width,int height);
void bitmapArgbToRgba(const uint8_t *data,int width,int height,uint8_t *out);
void bitmapArgbToBgra(uint8_t *data,int width,int height);
void bitmapArgbToBgra(const uint8_t *data,int width,int height,uint8_t *out);

void bitmapRgbToYuv420(const uint8_t *data,int width,int height,int channels,uint8_t *out);
void bitmapBgrToYuv420(const uint8_t *data,int width,int height,int channels,uint8_t *out);
void bitmapYuv420ToRgb(const uint8_t *data,int width,int height,uint8_t *out);

bool bitmapIsFullAlpha(const uint8_t *data,int width,int height);

}
