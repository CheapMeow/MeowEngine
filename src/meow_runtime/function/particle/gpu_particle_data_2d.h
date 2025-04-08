#pragma once

#include <glm/glm.hpp>

namespace Meow
{
    struct GPUParticleData2D
    {
        glm::vec2 position;
        glm::vec2 velocity;
        glm::vec4 color;
    };
} // namespace Meow
