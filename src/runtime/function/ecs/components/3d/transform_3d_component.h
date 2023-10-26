#pragma once

#include "function/ecs/component.h"

#include <glm/glm.hpp>

namespace Meow
{
    class Transform3DComponent : Component
    {
    public:
        glm::mat4 m_local_transform  = glm::mat4(1.0);
        glm::mat4 m_global_transform = glm::mat4(1.0);
    }
} // namespace Meow