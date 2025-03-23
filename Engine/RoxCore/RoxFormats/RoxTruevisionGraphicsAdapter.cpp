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

namespace RoxFormats
{

    std::size_t TGA::decodeHeader(const void* data, std::size_t size)
    {
        *this = TGA();

        if (!data || size < TGA::header_size)
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

        COLOR_MODE channels;
        bool rle = false;

        switch (bitsPerPixel)
        {
        case 32:
            if (dataTypeCode == 10)
                channels = COLOR_MODE::BGRA, rle = true;
            else if (dataTypeCode == 2)
                channels = COLOR_MODE::BGRA;
            else
                return 0;
            break;

        case 24:
            if (dataTypeCode == 10)
                channels = COLOR_MODE::BGR, rle = true;
            else if (dataTypeCode == 2)
                channels = COLOR_MODE::BGR;
            else
                return 0;
            break;

        case 8:
            if (dataTypeCode == 11)
                channels = COLOR_MODE::GREYSCALE, rle = true;
            else if (dataTypeCode == 3)
                channels = COLOR_MODE::GREYSCALE;
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
        this->horisontal_flip = (imageDescriptor & 0x10) != 0;
        this->vertical_flip = (imageDescriptor & 0x20) != 0;
        this->data = reader.getData();
        this->compressed_size = reader.getRemained();
        this->uncompressed_size = colorDataSize;

        return reader.getOffset();
    }

    std::size_t TGA::encodeHeader(void* to_data, std::size_t to_size)
    {
        if (to_size < TGA::header_size)
            return 0;

        char dataTypeCode = 0;
        char bpp = static_cast<char>(static_cast<int>(channels) * 8);

        dataTypeCode = rle ? 10 : 2;
        if (channels == COLOR_MODE::GREYSCALE)
            ++dataTypeCode;

        if (!dataTypeCode)
            return 0;

        unsigned char imageDescriptor = 0;
        if (horisontal_flip)
            imageDescriptor |= 0x10;
        if (vertical_flip)
            imageDescriptor |= 0x20;

        unsigned short w = static_cast<unsigned short>(width), h = static_cast<unsigned short>(height);
        char* out = static_cast<char*>(to_data);
        std::memset(out, 0, TGA::header_size);
        std::memcpy(out + 2, &dataTypeCode, 1);
        std::memcpy(out + 12, &w, 2);
        std::memcpy(out + 14, &h, 2);
        std::memcpy(out + 16, &bpp, 1);
        std::memcpy(out + 17, &imageDescriptor, 1);

        return TGA::header_size;
    }

    bool TGA::decodeRLE(void* decoded_data) const
    {
        if (!decoded_data || !rle)
            return false;

        using uchar = unsigned char;
        const uchar* cur = static_cast<const uchar*>(data);
        const uchar* const last = cur + compressed_size;
        uchar* out = static_cast<uchar*>(decoded_data);
        const uchar* const outLast = out + uncompressed_size;

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

    std::size_t TGA::encodeRLE(void* to_data, std::size_t to_size)
    {
        using uchar = unsigned char;

        const uchar* from = static_cast<const uchar*>(data);
        const uchar* fromLast = from + uncompressed_size;
        uchar* to = static_cast<uchar*>(to_data);
        uchar* toLast = to + to_size;

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

        return to - static_cast<uchar*>(to_data);
    }

    void TGA::flipVertical(const void* from_data, void* to_data)
    {
        if (!from_data || !to_data || height == 0)
            return;

        const int lineSize = width * static_cast<int>(channels);
        const int top = lineSize * (height - 1);

        using uchar = unsigned char;

        uchar* to = static_cast<uchar*>(to_data);

        if (from_data == to_data)
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
            const uchar* from = static_cast<const uchar*>(from_data);
            to += top;
            for (std::size_t offset = 0; offset < uncompressed_size; offset += lineSize)
                std::memcpy(to - offset, from + offset, lineSize);
        }
    }

    void TGA::flipHorisontal(const void* from_data, void* to_data)
    {
        if (!from_data || !to_data)
            return;

        const int lineSize = width * static_cast<int>(channels);
        const int half = lineSize / 2;

        using uchar = unsigned char;

        uchar* to = static_cast<uchar*>(to_data);

        if (from_data == to_data)
        {
            uchar tmp[4]; // Assuming maximum channels = 4

            for (std::size_t offset = 0; offset < uncompressed_size; offset += lineSize)
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
            const uchar* from = static_cast<const uchar*>(from_data);

            for (std::size_t offset = 0; offset < uncompressed_size; offset += lineSize)
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

    bool TgaFile::load(const char* file_name)
    {
        release();

        RoxResources::IRoxResourceData* inData = RoxResources::getResourcesProvider().access(file_name);
        if (!inData)
        {
            std::printf("Unable to open texture %s\n", file_name);
            return false;
        }

        RoxMemory::RoxTmpBufferScoped inBuf(inData->getSize());
        inData->readAll(inBuf.getData());
        const bool headerDecoded = m_header.decodeHeader(inBuf.getData(), inData->getSize()) > 0;
        inData->release();
        if (!headerDecoded)
            return false;

        if (m_header.rle)
        {
            m_data.resize(m_header.compressed_size);
            std::memcpy(&m_data[0], m_header.data, m_header.compressed_size);
        }
        else
        {
            m_data.resize(m_header.uncompressed_size);
            std::memcpy(&m_data[0], m_header.data, m_header.uncompressed_size);
        }

        return true;
    }

    bool TgaFile::create(int width, int height, TGA::COLOR_MODE channels, const void* data)
    {
        release();

        if (width <= 0 || height <= 0)
            return false;

        m_header.width = width;
        m_header.height = height;
        m_header.channels = channels;

        m_data.resize(width * height * static_cast<int>(channels), 0);
        if (data)
            std::memcpy(&m_data[0], data, m_data.size());

        return true;
    }

    bool TgaFile::decodeRLE()
    {
        if (m_data.empty())
            return false;

        if (!m_header.rle)
            return false;

        m_header.data = &m_data[0];
        RoxMemory::RoxTmpBufferScoped buf(m_header.uncompressed_size);
        if (!m_header.decodeRLE(buf.getData()))
            return false;

        m_header.rle = false;
        m_data.resize(m_header.uncompressed_size);
        std::memcpy(&m_data[0], buf.getData(), m_header.uncompressed_size);

        return true;
    }

    bool TgaFile::encodeRLE(std::size_t max_compressed_size)
    {
        if (m_data.empty())
            return false;

        if (m_header.rle)
            return false;

        m_header.data = &m_data[0];
        RoxMemory::RoxTmpBufferScoped buf(max_compressed_size);
        std::size_t encodedSize = m_header.encodeRLE(buf.getData(), max_compressed_size);
        if (!encodedSize)
            return false;

        m_header.rle = true;
        m_header.compressed_size = encodedSize;
        m_data.resize(encodedSize);
        std::memcpy(&m_data[0], buf.getData(), encodedSize);

        return true;
    }

    bool TgaFile::save(const char* file_name)
    {
        if (m_data.empty())
            return false;

        FILE* outFile = std::fopen(file_name, "wb");
        if (!outFile)
        {
            std::printf("Unable to save texture %s\n", file_name);
            return false;
        }

        char headerBuf[m_header.header_size];
        std::size_t header_size = m_header.encodeHeader(headerBuf, sizeof(headerBuf));
        std::fwrite(headerBuf, 1, header_size, outFile);
        std::fwrite(&m_data[0], 1, m_data.size(), outFile);

        std::fclose(outFile);
        return true;
    }

    bool TgaFile::flipHorizontal()
    {
        if (m_data.empty())
            return false;

        if (m_header.rle && !decodeRLE())
            return false;

        m_header.flipHorisontal(&m_data[0], &m_data[0]);
        m_header.horisontal_flip = !m_header.horisontal_flip;

        return true;
    }

    bool TgaFile::flipVertical()
    {
        if (m_data.empty())
            return false;

        if (m_header.rle && !decodeRLE())
            return false;

        m_header.flipVertical(&m_data[0], &m_data[0]);
        m_header.vertical_flip = !m_header.vertical_flip;

        return true;
    }

} // namespace RoxFormats
