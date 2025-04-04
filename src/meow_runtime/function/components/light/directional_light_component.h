#pragma once

#include "core/reflect/macros.h"
#include "function/object/game_object.h"

#include <glm/glm.hpp>

namespace Meow
{
    class [[reflectable_class()]] DirectionalLightComponent : public Component
    {
    public:
        [[reflectable_field()]]
        float field_of_view = glm::radians(45.0f);

        [[reflectable_field()]]
        float near_plane = 0.1f;

        [[reflectable_field()]]
        float far_plane = 1000.0f;
    };
} // namespace Meow
