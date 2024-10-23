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

#include "RoxAnim.h"
#include "RoxMemory/RoxMemoryReader.h"
#include "RoxMemory/RoxMemoryWriter.h"
#include <cstdio> 
#include <cstdint>
#include <cstring>

namespace { const char ran_sign[] = { 'r','o','x',' ','a','n','i','m' }; }

namespace RoxFormats
{

    void readFrame(RoxAnim::PosVec3LinearFrame& f, RoxMemory::RoxMemoryReader& reader)
    {
        f.time = reader.read<uint32_t>();
        f.pos = reader.read<RoxMath::Vector3>();
    }

    void writeFrame(const RoxAnim::PosVec3LinearFrame& f, RoxMemory::RoxMemoryWriter& writer)
    {
        writer.writeUint(f.time);
        writer.writeFloat(f.pos.x);
        writer.writeFloat(f.pos.y);
        writer.writeFloat(f.pos.z);
    }

    void readFrame(RoxAnim::RotQuatLinearFrame& f, RoxMemory::RoxMemoryReader& reader)
    {
        f.time = reader.read<uint32_t>();
        f.rot = reader.read<RoxMath::Quaternion>();
    }

    void writeFrame(const RoxAnim::RotQuatLinearFrame& f, RoxMemory::RoxMemoryWriter& writer)
    {
        writer.writeUint(f.time);
        writer.writeFloat(f.rot.v.x);
        writer.writeFloat(f.rot.v.y);
        writer.writeFloat(f.rot.v.z);
        writer.writeFloat(f.rot.w);
    }

    void readFrame(RoxAnim::FloatLinearFrame& f, RoxMemory::RoxMemoryReader& reader)
    {
        f.time = reader.read<uint32_t>();
        f.value = reader.read<float>();
    }

    void writeFrame(const RoxAnim::FloatLinearFrame& f, RoxMemory::RoxMemoryWriter& writer)
    {
        writer.writeUint(f.time);
        writer.writeFloat(f.value);
    }

    template <typename T>
    void readCurves(std::vector<RoxAnim::Curve<T>>& array, RoxMemory::RoxMemoryReader& reader)
    {
        const uint32_t curvesCount = reader.read<uint32_t>();
        array.resize(curvesCount);
        for (uint32_t i = 0; i < curvesCount; ++i)
        {
            array[i].boneName = reader.readString();
            const uint32_t framesCount = reader.read<uint32_t>();
            array[i].frames.resize(framesCount);
            for (uint32_t j = 0; j < framesCount; ++j)
                readFrame(array[i].frames[j], reader);
        }
    }

    template <typename T>
    void writeCurves(const std::vector<RoxAnim::Curve<T>>& array, RoxMemory::RoxMemoryWriter& writer)
    {
        const uint32_t curvesCount = static_cast<uint32_t>(array.size());
        writer.writeUint(curvesCount);
        for (uint32_t i = 0; i < curvesCount; ++i)
        {
            writer.writeString(array[i].boneName);
            const uint32_t framesCount = static_cast<uint32_t>(array[i].frames.size());
            writer.writeUint(framesCount);
            for (uint32_t j = 0; j < framesCount; ++j)
                writeFrame(array[i].frames[j], writer);
        }
    }

    bool RoxAnim::read(const void* data, std::size_t size)
    {
        *this = RoxAnim();

        if (!data || size == 0)
            return false;

        RoxMemory::RoxMemoryReader reader(data, size);

        if (!reader.test(ran_sign, std::strlen(ran_sign)))
            return false;

        version = reader.read<uint32_t>();
        if (version != 1)
            return false;

        readCurves(posVec3LinearCurves, reader);
        readCurves(rotQuatLinearCurves, reader);
        readCurves(floatLinearCurves, reader);

        return true;
    }

    std::size_t RoxAnim::write(void* data, std::size_t size) const
    {
        RoxMemory::RoxMemoryWriter writer(data, size);
        writer.write(ran_sign, std::strlen(ran_sign));
        writer.writeUint(version);

        writeCurves(posVec3LinearCurves, writer);
        writeCurves(rotQuatLinearCurves, writer);
        writeCurves(floatLinearCurves, writer);

        return writer.getOffset();
    }

} // namespace RoxFormats
