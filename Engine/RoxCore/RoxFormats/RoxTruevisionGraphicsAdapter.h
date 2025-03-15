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

#include <vector>
#include <cstddef>

#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"

namespace RoxFormats
{

    struct RTga
    {
        enum COLORMODE
        {
            GREYSCALE = 1,
            BGR = 3,
            BGRA = 4
        };

        int width;
        int height;
        COLORMODE channels;
        bool rle;
        bool horisontalFlip;
        bool verticalFlip;

        const void* data;
        std::size_t compressedSize;
        std::size_t uncompressedSize;

    public:
        std::size_t decodeHeader(const void* data, std::size_t size); // Returns 0 if invalid
        bool decodeRle(void* decodedData) const; // decodedData must be allocated with uncompressedSize
        void flipHorisontal(const void* fromData, void* toData); // toData must be allocated; can be equal to fromData
        void flipVertical(const void* fromData, void* toData);

    public:
        std::size_t encodeHeader(void* toData, std::size_t toSize = RTga::headerSize);
        std::size_t encodeRle(void* toData, std::size_t toSize); // toData size should be allocated with enough size

    public:
        static const std::size_t headerSize = 18;
        static const std::size_t minimumHeaderSize = headerSize; // ToDo: remove legacy

    public:
        RTga()
            : width(0), height(0), rle(false), horisontalFlip(false),
            verticalFlip(false), data(nullptr), compressedSize(0), uncompressedSize(0) {}
    };

    class RTgaFile
    {
    public:
        bool load(const char* fileName);
        bool create(int width, int height, RTga::COLORMODE channels, const void* data);
        bool decodeRle();
        bool encodeRle(std::size_t maxCompressedSize);
        bool save(const char* fileName);
        bool flipHorizontal();
        bool flipVertical();
        void release() { mHeader = RTga(); mData.clear(); }

    public:
        bool isRle() const { return mHeader.rle; }
        bool isFlippedVertical() const { return mHeader.verticalFlip; }
        bool isFlippedHorizontal() const { return mHeader.horisontalFlip; }
        int getWidth() const { return mHeader.width; }
        int getHeight() const { return mHeader.height; }
        RTga::COLORMODE getChannels() const { return mHeader.channels; }
        const unsigned char* getData() const { return mData.empty() ? nullptr : &mData[0]; }
        unsigned char* getData() { return mData.empty() ? nullptr : &mData[0]; }
        std::size_t getDataSize() const { return mData.size(); }

    private:
        RTga mHeader;
        std::vector<unsigned char> mData;
    };

}
