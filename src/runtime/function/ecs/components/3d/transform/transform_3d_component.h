#pragma once

#include "function/ecs/component.h"

#include <glm/glm.hpp>

namespace Meow
{
    struct Transform3DComponent : Component
    {
        glm::mat4 m_local_transform  = glm::mat4(1.0f);
        glm::mat4 m_global_transform = glm::mat4(1.0f);
    };
} // namespace Meow