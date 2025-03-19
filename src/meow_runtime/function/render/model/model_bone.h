#pragma once

#include <string>

#include <glm/glm.hpp>

namespace Meow
{
    struct ModelBone
    {
        std::string name;
        size_t      index  = -1;
        size_t      parent = -1;
        glm::mat4   inverse_bind_pose;
        glm::mat4   final_transform;
    };

    struct ModelVertexSkin
    {
        size_t used = 0;
        size_t indices[4];
        float  weights[4];
    };
} // namespace Meow