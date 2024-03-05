#pragma once

#include <glm/glm.hpp>

namespace Meow
{
    /**
     * @brief Temp definition of UBO
     *
     * UBO struct should be defined from analysis from shader files?
     */
    struct UBOData
    {
        glm::mat4 model      = glm::mat4(1.0f);
        glm::mat4 view       = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
    };
} // namespace Meow