// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "RoxMesh.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxMemory/RoxMemoryWriter.h"
#include "RoxMemory/RoxInvalidObject.h"
#include <cstdio> 
#include <cstdint>
#include <cstring>

namespace { const char rms_sign[] = { 'r','o','x',' ','m','e','s','h' }; } // Updated signature

namespace RoxFormats
{

    bool RMesh::readChunksInfo(const void* data, std::size_t size)
    {
        *this = RMesh();

        if (!data || !size)
            return false;

        const char* cdata = static_cast<const char*>(data);
        const char* const data_end = cdata + size;
        Header h;
        cdata += readHeader(h, data, size);
        if (cdata == data)
            return false;

        version = h.version;
        chunks.resize(h.chunksCount);
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            cdata += readChunkInfo(chunks[i], cdata, data_end - cdata);
            cdata += chunks[i].size;
            if (cdata > data_end)
            {
                *this = RMesh();
                return false;
            }
        }

        return true;
    }

    std::size_t RMesh::readHeader(Header& outHeader, const void* data, std::size_t size)
    {
        outHeader.version = outHeader.chunksCount = 0;
        if (size < header_size)
            return 0;

        RoxMemory::RoxMemoryReader reader(data, size);
        if (!reader.test(rms_sign, std::strlen(rms_sign)))
            return 0;

        outHeader.version = reader.read<uint32_t>();
        if (!outHeader.version)
            return 0;

        outHeader.chunksCount = reader.read<uint32_t>();

        return reader.getOffset();
    }

    std::size_t RMesh::readChunkInfo(ChunkInfo& outChunkInfo, const void* data, std::size_t size)
    {
        if (size < sizeof(uint32_t) * 2)
            return 0;

        outChunkInfo.type = outChunkInfo.size = 0;
        RoxMemory::RoxMemoryReader reader(data, size);
        outChunkInfo.type = reader.read<uint32_t>();
        outChunkInfo.size = reader.read<uint32_t>();
        outChunkInfo.data = reader.getData();

        return reader.getOffset();
    }

    std::size_t RMesh::getRMeshSize() const
    {
        std::size_t size = header_size;
        for (size_t i = 0; i < chunks.size(); ++i)
            size += getChunkWriteSize(chunks[i].size);

        return size;
    }

    std::size_t RMesh::writeToBuffer(void* data, std::size_t size) const
    {
        if (!data)
            return 0;

        char* cdata = static_cast<char*>(data);
        const char* const data_end = cdata + size;

        Header h;
        h.version = version;
        h.chunksCount = static_cast<unsigned int>(chunks.size());
        cdata += writeHeaderToBuffer(h, data, size);
        if (data == cdata)
            return 0;

        for (size_t i = 0; i < chunks.size(); ++i)
            cdata += writeChunkToBuffer(chunks[i], cdata, data_end - cdata);

        return cdata - static_cast<char*>(data);
    }

    std::size_t RMesh::writeHeaderToBuffer(const Header& h, void* toData, std::size_t toSize)
    {
        if (!toData || toSize < header_size)
            return 0;

        RoxMemory::RoxMemoryWriter writer(toData, toSize);
        writer.write(rms_sign, std::strlen(rms_sign));
        writer.writeUint(h.version);
        writer.writeUint(h.chunksCount);

        return writer.getOffset();
    }

    std::size_t RMesh::writeHeaderToBuffer(unsigned int chunksCount, void* toData, std::size_t toSize)
    {
        Header h;
        h.version = latest_version;
        h.chunksCount = chunksCount;
        return writeHeaderToBuffer(h, toData, toSize);
    }

    std::size_t RMesh::getChunkWriteSize(std::size_t chunkDataSize)
    {
        return chunkDataSize + sizeof(uint32_t) * 2;
    }

    std::size_t RMesh::writeChunkToBuffer(const ChunkInfo& chunk, void* toData, std::size_t toSize)
    {
        if (!toData || toSize < getChunkWriteSize(chunk.size))
            return 0;

        RoxMemory::RoxMemoryWriter writer(toData, toSize);
        writer.writeUint(chunk.type);
        writer.writeUint(chunk.size);
        if (!chunk.size)
            return writer.getOffset();

        if (!chunk.data)
            return 0;

        writer.write(chunk.data, chunk.size);
        return writer.getOffset();
    }

    // The rest of the code (e.g., RMeshMeshChunk, RMeshMaterialChunk, etc.) should be similarly renamed from `nms_` to `RMesh_` and updated accordingly.

} // namespace RoxFormats
