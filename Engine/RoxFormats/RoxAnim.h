// nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#pragma once

#include <vector>
#include <string>
#include <cstddef> 
#include "RoxMath/RoxVector.h"
#include "RoxMath/RoxQuaternion.h"

namespace RoxFormats
{

    struct RoxAnim
    {
        unsigned int version;

        template <typename T>
        struct Curve
        {
            std::string boneName;
            std::vector<T> frames;
        };

        struct PosVec3LinearFrame
        {
            unsigned int time;
            RoxMath::Vector3 pos;
        };
        std::vector<Curve<PosVec3LinearFrame>> posVec3LinearCurves;

        struct RotQuatLinearFrame
        {
            unsigned int time;
            RoxMath::Quaternion rot;
        };
        std::vector<Curve<RotQuatLinearFrame>> rotQuatLinearCurves;

        struct FloatLinearFrame
        {
            unsigned int time;
            float value;
        };
        std::vector<Curve<FloatLinearFrame>> floatLinearCurves;

        template <typename T>
        static T& addCurve(std::vector<T>& curves, const std::string& name)
        {
            for (size_t i = 0; i < curves.size(); ++i)
            {
                if (curves[i].boneName == name)
                    return curves[i];
            }

            curves.emplace_back();
            curves.back().boneName = name;
            return curves.back();
        }

        RoxAnim() : version(0) {}

    public:
        bool read(const void* data, std::size_t size);
        std::size_t write(void* toData, std::size_t toSize) const; // toSize = getSize()
        std::size_t getSize() const { return write(nullptr, 0); }
    };

} // namespace RoxFormats
