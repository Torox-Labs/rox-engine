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

#include "RoxTruevisionGraphicsAdapter.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxMemory/RoxTmpBuffers.h"
#include "RoxResources/RoxResources.h"
#include <cstdio> 
#include <cstdint>
#include <cstring>

namespace { const char roxt_sign[] = { 'r','o','x','t','_','t','g','a' }; } // Updated signature

namespace RoxFormats
{

    std::size_t RTga::decodeHeader(const void* data, std::size_t size)
    {
        *this = RTga();

        if (!data || size < RTga::headerSize)
            return 0;

        RoxMemory::RoxMemoryReader reader(data, size);

        const char idLength = reader.read<char>();
        if (!reader.skip(idLength))
            return 0;

        const char colorMapType = reader.read<char>();
        if (colorMapType != 0)
            return 0;

        const char dataTypeCode = reader.read<char>();
        const short colorMapOrigin = reader.read<short>();
        if (colorMapOrigin != 0)
            return 0;

        const short colorMapLength = reader.read<short>();
        if (colorMapLength != 0)
            return 0;

        const char colorMapDepth = reader.read<char>();
        if (colorMapDepth != 0)
            return 0;

        // const short x_origin =
        reader.read<short>();
        // const short y_origin =
        reader.read<short>();

        const short width = reader.read<short>();
        const short height = reader.read<short>();
        const char bitsPerPixel = reader.read<char>();
        const unsigned char imageDescriptor = reader.read<char>();

        COLORMODE channels;
        bool rle = false;

        switch (bitsPerPixel)
        {
        case 32:
            if (dataTypeCode == 10)
                channels = COLORMODE::BGRA, rle = true;
            else if (dataTypeCode == 2)
                channels = COLORMODE::BGRA;
            else
                return 0;
            break;

        case 24:
            if (dataTypeCode == 10)
                channels = COLORMODE::BGR, rle = true;
            else if (dataTypeCode == 2)
                channels = COLORMODE::BGR;
            else
                return 0;
            break;

        case 8:
            if (dataTypeCode == 11)
                channels = COLORMODE::GREYSCALE, rle = true;
            else if (dataTypeCode == 3)
                channels = COLORMODE::GREYSCALE;
            else
                return 0;
            break;

        default:
            return 0;
        }

        const std::size_t colorDataSize = width * height * static_cast<int>(channels);
        if (!colorDataSize)
            return 0;

        this->width = width;
        this->height = height;
        this->channels = channels;
        this->rle = rle;
        this->horisontalFlip = (imageDescriptor & 0x10) != 0;
        this->verticalFlip = (imageDescriptor & 0x20) != 0;
        this->data = reader.getData();
        this->compressedSize = reader.getRemained();
        this->uncompressedSize = colorDataSize;

        return reader.getOffset();
    }

    std::size_t RTga::encodeHeader(void* toData, std::size_t toSize)
    {
        if (toSize < RTga::headerSize)
            return 0;

        char dataTypeCode = 0;
        char bpp = static_cast<char>(static_cast<int>(channels) * 8);

        dataTypeCode = rle ? 10 : 2;
        if (channels == COLORMODE::GREYSCALE)
            ++dataTypeCode;

        if (!dataTypeCode)
            return 0;

        unsigned char imageDescriptor = 0;
        if (horisontalFlip)
            imageDescriptor |= 0x10;
        if (verticalFlip)
            imageDescriptor |= 0x20;

        unsigned short w = static_cast<unsigned short>(width), h = static_cast<unsigned short>(height);
        char* out = static_cast<char*>(toData);
        std::memset(out, 0, RTga::headerSize);
        std::memcpy(out + 2, &dataTypeCode, 1);
        std::memcpy(out + 12, &w, 2);
        std::memcpy(out + 14, &h, 2);
        std::memcpy(out + 16, &bpp, 1);
        std::memcpy(out + 17, &imageDescriptor, 1);

        return RTga::headerSize;
    }

    bool RTga::decodeRle(void* decodedData) const
    {
        if (!decodedData || !rle)
            return false;

        using uchar = unsigned char;
        const uchar* cur = static_cast<const uchar*>(data);
        const uchar* const last = cur + compressedSize;
        uchar* out = static_cast<uchar*>(decodedData);
        const uchar* const outLast = out + uncompressedSize;

        while (out < outLast)
        {
            if (cur >= last)
                return false;

            if (*cur & 0x80)
            {
                const uchar* to = out + (*cur++ - 127) * static_cast<int>(channels);
                if (cur + static_cast<int>(channels) > last || to > outLast)
                    return false;

                while (out < to)
                {
                    std::memcpy(out, cur, static_cast<int>(channels));
                    out += static_cast<int>(channels);
                }
                cur += static_cast<int>(channels);
            }
            else // raw
            {
                const std::size_t size = (*cur++ + 1) * static_cast<int>(channels);
                if (cur + size > last || out + size > outLast)
                    return false;

                std::memcpy(out, cur, size);
                cur += size;
                out += size;
            }
        }

        return true;
    }

    std::size_t RTga::encodeRle(void* toData, std::size_t toSize)
    {
        using uchar = unsigned char;

        const uchar* from = static_cast<const uchar*>(data);
        const uchar* fromLast = from + uncompressedSize;
        uchar* to = static_cast<uchar*>(toData);
        uchar* toLast = to + toSize;

        uchar raw[128 * 4];
        std::memset(raw, 0, sizeof(raw));

        int currentLine = 0;
        for (int runLength = 1; from < fromLast; from += static_cast<int>(channels) * runLength, currentLine += runLength)
        {
            if (currentLine >= width)
                currentLine -= width;

            std::memcpy(raw, from, static_cast<int>(channels));

            runLength = 1;
            bool isRle = false;
            int maxRunLength = 128;
            if (currentLine + maxRunLength > width)
                maxRunLength = width - currentLine;

            const uchar* check = from + channels;
            for (; runLength < maxRunLength && check < fromLast; ++runLength, check += channels)
            {
                if (std::memcmp(raw, check, channels) != 0)
                {
                    isRle = runLength > 1;
                    break;
                }
            }

            if (isRle)
            {
                if (to + channels + 1 > toLast)
                    return 0;

                *to++ = static_cast<uchar>(128 | (runLength - 1));
                std::memcpy(to, raw, channels);
                to += channels;
                continue;
            }

            runLength = 1;
            uchar* rawIt = raw;
            for (const uchar* tempCheck = from + channels; tempCheck < fromLast; ++runLength, tempCheck += channels)
            {
                if ((std::memcmp(rawIt, tempCheck, channels) != 0 && runLength < maxRunLength) || runLength < 3)
                {
                    std::memcpy(rawIt += channels, tempCheck, channels);
                    if (runLength >= maxRunLength)
                        break;

                    continue;
                }

                // Check if the exit condition was the start of a repeating color
                if (std::memcmp(rawIt, tempCheck, channels) != 0)
                    runLength -= 2;

                break;
            }

            std::size_t rawSize = channels * runLength;
            if (to + rawSize + 1 > toLast)
                return 0;

            *to++ = static_cast<uchar>(runLength - 1);
            std::memcpy(to, raw, rawSize);
            to += rawSize;
        }

        return to - static_cast<uchar*>(toData);
    }

    void RTga::flipVertical(const void* fromData, void* toData)
    {
        if (!fromData || !toData || height == 0)
            return;

        const int lineSize = width * static_cast<int>(channels);
        const int top = lineSize * (height - 1);

        using uchar = unsigned char;

        uchar* to = static_cast<uchar*>(toData);

        if (fromData == toData)
        {
            if (!lineSize)
                return;

            const int half = lineSize * (height / 2);
            std::vector<uchar> lineData(lineSize);
            uchar* line = lineData.data();
            uchar* from = to;
            to += top;

            for (int offset = 0; offset < half; offset += lineSize)
            {
                uchar* f = from + offset;
                uchar* t = to - offset;
                std::memcpy(line, f, lineSize);
                std::memcpy(f, t, lineSize);
                std::memcpy(t, line, lineSize);
            }
        }
        else
        {
            const uchar* from = static_cast<const uchar*>(fromData);
            to += top;
            for (std::size_t offset = 0; offset < uncompressedSize; offset += lineSize)
                std::memcpy(to - offset, from + offset, lineSize);
        }
    }

    void RTga::flipHorisontal(const void* fromData, void* toData)
    {
        if (!fromData || !toData)
            return;

        const int lineSize = width * static_cast<int>(channels);
        const int half = lineSize / 2;

        using uchar = unsigned char;

        uchar* to = static_cast<uchar*>(toData);

        if (fromData == toData)
        {
            uchar tmp[4]; // Assuming maximum channels = 4

            for (std::size_t offset = 0; offset < uncompressedSize; offset += lineSize)
            {
                uchar* ha = to + offset;
                uchar* hb = ha + lineSize - static_cast<int>(channels);

                for (int w = 0; w < half; w += static_cast<int>(channels))
                {
                    uchar* a = ha + w;
                    uchar* b = hb - w;
                    std::memcpy(tmp, a, channels);
                    std::memcpy(a, b, channels);
                    std::memcpy(b, tmp, channels);
                }
            }
        }
        else
        {
            const uchar* from = static_cast<const uchar*>(fromData);

            for (std::size_t offset = 0; offset < uncompressedSize; offset += lineSize)
            {
                const uchar* ha = from + offset;
                uchar* hb = to + offset + lineSize - static_cast<int>(channels);

                for (int w = 0; w < lineSize; w += static_cast<int>(channels))
                {
                    const uchar* a = ha + w;
                    uchar* b = hb - w;
                    std::memcpy(b, a, channels);
                }
            }
        }
    }

    bool RTgaFile::load(const char* fileName)
    {
        release();

        RoxResources::IRoxResourceData* inData = RoxResources::getResourcesProvider().access(fileName);
        if (!inData)
        {
            std::printf("Unable to open texture %s\n", fileName);
            return false;
        }

        RoxMemory::RoxTmpBufferScoped inBuf(inData->getSize());
        inData->readAll(inBuf.getData());
        const bool headerDecoded = mHeader.decodeHeader(inBuf.getData(), inData->getSize()) > 0;
        inData->release();
        if (!headerDecoded)
            return false;

        if (mHeader.rle)
        {
            mData.resize(mHeader.compressedSize);
            std::memcpy(&mData[0], mHeader.data, mHeader.compressedSize);
        }
        else
        {
            mData.resize(mHeader.uncompressedSize);
            std::memcpy(&mData[0], mHeader.data, mHeader.uncompressedSize);
        }

        return true;
    }

    bool RTgaFile::create(int width, int height, RTga::COLORMODE channels, const void* data)
    {
        release();

        if (width <= 0 || height <= 0)
            return false;

        mHeader.width = width;
        mHeader.height = height;
        mHeader.channels = channels;

        mData.resize(width * height * static_cast<int>(channels), 0);
        if (data)
            std::memcpy(&mData[0], data, mData.size());

        return true;
    }

    bool RTgaFile::decodeRle()
    {
        if (mData.empty())
            return false;

        if (!mHeader.rle)
            return false;

        mHeader.data = &mData[0];
        RoxMemory::RoxTmpBufferScoped buf(mHeader.uncompressedSize);
        if (!mHeader.decodeRle(buf.getData()))
            return false;

        mHeader.rle = false;
        mData.resize(mHeader.uncompressedSize);
        std::memcpy(&mData[0], buf.getData(), mHeader.uncompressedSize);

        return true;
    }

    bool RTgaFile::encodeRle(std::size_t maxCompressedSize)
    {
        if (mData.empty())
            return false;

        if (mHeader.rle)
            return false;

        mHeader.data = &mData[0];
        RoxMemory::RoxTmpBufferScoped buf(maxCompressedSize);
        std::size_t encodedSize = mHeader.encodeRle(buf.getData(), maxCompressedSize);
        if (!encodedSize)
            return false;

        mHeader.rle = true;
        mHeader.compressedSize = encodedSize;
        mData.resize(encodedSize);
        std::memcpy(&mData[0], buf.getData(), encodedSize);

        return true;
    }

    bool RTgaFile::save(const char* fileName)
    {
        if (mData.empty())
            return false;

        FILE* outFile = std::fopen(fileName, "wb");
        if (!outFile)
        {
            std::printf("Unable to save texture %s\n", fileName);
            return false;
        }

        char headerBuf[mHeader.headerSize];
        std::size_t headerSize = mHeader.encodeHeader(headerBuf, sizeof(headerBuf));
        std::fwrite(headerBuf, 1, headerSize, outFile);
        std::fwrite(&mData[0], 1, mData.size(), outFile);

        std::fclose(outFile);
        return true;
    }

    bool RTgaFile::flipHorizontal()
    {
        if (mData.empty())
            return false;

        if (mHeader.rle && !decodeRle())
            return false;

        mHeader.flipHorisontal(&mData[0], &mData[0]);
        mHeader.horisontalFlip = !mHeader.horisontalFlip;

        return true;
    }

    bool RTgaFile::flipVertical()
    {
        if (mData.empty())
            return false;

        if (mHeader.rle && !decodeRle())
            return false;

        mHeader.flipVertical(&mData[0], &mData[0]);
        mHeader.verticalFlip = !mHeader.verticalFlip;

        return true;
    }

} // namespace RoxFormats
