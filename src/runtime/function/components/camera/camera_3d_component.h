#pragma once

#include "core/reflect/macros.h"
#include "function/components/component.h"
#include "function/components/transform/transform_3d_component.h"

#include <glm/glm.hpp>

namespace Meow
{
    enum class CameraMode : unsigned char
    {
        ThirdPerson,
        FirstPerson,
        Free,
        Invalid
    };

    class [[reflectable_class()]] Camera3DComponent : public Component
    {
    public:
        [[reflectable_field()]]
        float near_plane = 0.1f;

        [[reflectable_field()]]
        float far_plane = 1000.0f;

        [[reflectable_field()]]
        float field_of_view = glm::radians(45.0f);

        [[reflectable_field()]]
        CameraMode camera_mode = CameraMode::Invalid;

        void Start() override;

        void Tick(float dt) override;

    private:
        std::weak_ptr<Transform3DComponent> m_transform;

        void TickFreeCamera(float dt);
    };
} // namespace Meow
