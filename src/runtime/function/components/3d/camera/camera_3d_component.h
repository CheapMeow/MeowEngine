#pragma once

#include "core/reflect/macros.h"
#include "function/components/component.h"

#include <glm/glm.hpp>

namespace Meow
{
    struct [[reflectable_struct()]] Camera3DComponent : Component
    {
        [[reflectable_field()]]
        bool is_main_camera = false;

        [[reflectable_field()]]
        float near_plane = 0.1f;

        [[reflectable_field()]]
        float far_plane = 1000.0f;

        [[reflectable_field()]]
        float field_of_view = glm::radians(45.0f);
    };
} // namespace Meow
