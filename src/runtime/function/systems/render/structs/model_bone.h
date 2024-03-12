#pragma once

#include <string>

#include <glm/glm.hpp>

namespace Meow
{
    struct Bone
    {
        std::string name;
        size_t      index  = -1;
        size_t      parent = -1;
        glm::mat4   inverseBindPose;
        glm::mat4   finalTransform;
    };
} // namespace Meow