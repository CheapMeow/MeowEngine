#pragma once

#include <glm/glm.hpp>

namespace Meow
{
    struct PerSceneData
    {
        glm::mat4 view       = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
    };
} // namespace Meow