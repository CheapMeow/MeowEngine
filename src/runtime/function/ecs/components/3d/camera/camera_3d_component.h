#pragma once

#include "function/ecs/component.h"

#include <glm/glm.hpp>

namespace Meow
{
    struct Camera3DComponent : Component
    {
        bool is_main_camera = false;

        float near_plane = 0.1f, far_plane = 1000.0f;
        float field_of_view = glm::radians(70.0f);

        glm::mat4 view       = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
    };
} // namespace Meow
