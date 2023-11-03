#pragma once

#include "function/ecs/component.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Meow
{
    struct Camera3DComponent : Component
    {
        bool is_main_camera = false;

        float near_plane = 0.1f, far_plane = 1000.0f;
        float field_of_view = glm::radians(45.0f);

        float yaw = 0.0f, pitch = 0.0f;
    };
} // namespace Meow
