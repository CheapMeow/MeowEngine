#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Meow
{
    template<class ValueType>
    struct ModelAnimChannel
    {
        std::vector<float>     keys;
        std::vector<ValueType> values;

        void GetValue(float key, ValueType& outPrevValue, ValueType& outNextValue, float& outAlpha)
        {
            outAlpha = 0.0f;

            if (keys.size() == 0)
            {
                return;
            }

            if (key <= keys.front())
            {
                outPrevValue = values.front();
                outNextValue = values.front();
                outAlpha     = 0.0f;
                return;
            }

            if (key >= keys.back())
            {
                outPrevValue = values.back();
                outNextValue = values.back();
                outAlpha     = 0.0f;
                return;
            }

            size_t frameIndex = 0;
            for (size_t i = 0; i < keys.size() - 1; ++i)
            {
                if (key <= keys[i + 1])
                {
                    frameIndex = i;
                    break;
                }
            }

            outPrevValue = values[frameIndex + 0];
            outNextValue = values[frameIndex + 1];

            float prevKey = keys[frameIndex + 0];
            float nextKey = keys[frameIndex + 1];
            outAlpha      = (key - prevKey) / (nextKey - prevKey);
        }
    };

    struct ModelAnimationClip
    {
        std::string                 node_name;
        float                       duration;
        ModelAnimChannel<glm::vec3> positions;
        ModelAnimChannel<glm::vec3> scales;
        ModelAnimChannel<glm::quat> rotations;
    };

    struct ModelAnimation
    {
        std::string                                         name;
        float                                               time     = 0.0f;
        float                                               duration = 0.0f;
        float                                               speed    = 1.0f;
        std::unordered_map<std::string, ModelAnimationClip> clips;
    };
} // namespace Meow
