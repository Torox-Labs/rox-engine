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
#include <string>
#include <cstddef>
#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"

namespace RoxFormats
{

    struct RMesh
    {
        unsigned int version;

        struct ChunkInfo
        {
            unsigned int type;
            unsigned int size;
            const void* data;
        };

        enum SectionType
        {
            MESH_DATA,
            SKELETON,
            MATERIALS,
            GENERAL
        };

        std::vector<ChunkInfo> chunks;

        RMesh() : version(0) {}

    public:
        bool readChunksInfo(const void* data, std::size_t size);

    public:
        struct Header
        {
            unsigned int version;
            unsigned int chunksCount;
        };

        static std::size_t readHeader(Header& outHeader, const void* data, std::size_t size = RMesh::header_size);
        static std::size_t readChunkInfo(ChunkInfo& outChunkInfo, const void* data, std::size_t size);

    public:
        std::size_t getRMeshSize() const;
        std::size_t writeToBuffer(void* toData, std::size_t toSize) const; // toSize = getRMeshSize()

    public:
        static std::size_t writeHeaderToBuffer(const Header& h, void* toData, std::size_t toSize = RMesh::header_size);
        static std::size_t writeHeaderToBuffer(unsigned int chunksCount, void* toData, std::size_t toSize = RMesh::header_size);

        static std::size_t getChunkWriteSize(std::size_t chunkDataSize);
        static std::size_t writeChunkToBuffer(const ChunkInfo& chunk, void* toData, std::size_t toSize); // toSize = getChunkWriteSize()

    public:
        const static std::size_t header_size = 16;
        const static unsigned int latest_version = 2;
    };

    // Rest of the code remains similar with renaming from 'nms_' to 'RMesh_'

} // namespace RoxFormats
