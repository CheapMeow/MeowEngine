#include "camera_3d_component.hpp"

#include "core/math/math.h"
#include "function/global/runtime_global_context.h"
#include "function/object/game_object.h"

namespace Meow
{
    void Camera3DComponent::Start()
    {
        if (!m_parent_object.lock())
            RUNTIME_INFO("Not Found!");
        m_transform = m_parent_object.lock()->TryGetComponent<Transform3DComponent>("Transform3DComponent");
    }

    void Camera3DComponent::Tick(float dt)
    {
        if (camera_mode == CameraMode::Free)
        {
            TickFreeCamera(dt);
        }
    }

    void Camera3DComponent::TickFreeCamera(float dt)
    {
        if (std::shared_ptr<Transform3DComponent> transform_component = m_transform.lock())
        {
            if (g_runtime_global_context.input_system->GetButton("RightMouse")->GetAction() == InputAction::Press)
            {
                float dx = g_runtime_global_context.input_system->GetAxis("MouseX")->GetAmount();
                float dy = g_runtime_global_context.input_system->GetAxis("MouseY")->GetAmount();

                glm::vec3 temp_right = transform_component->rotation * glm::vec3(1.0f, 0.0f, 0.0f);

                // TODO: config camera rotate velocity
                glm::quat dyaw   = Math::QuaternionFromAngleAxis(-dx * dt * 100.0f, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::quat dpitch = Math::QuaternionFromAngleAxis(-dy * dt * 100.0f, temp_right);

                transform_component->rotation = dyaw * dpitch * transform_component->rotation;
            }

            glm::vec3 right   = transform_component->rotation * glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 forward = transform_component->rotation * glm::vec3(0.0f, 0.0f, 1.0f);
            glm::vec3 up      = glm::vec3(0.0f, 1.0f, 0.0f);

            glm::vec3 movement = glm::vec3(0.0f);

            if (g_runtime_global_context.input_system->GetButton("Left")->GetAction() == InputAction::Press)
            {
                movement += -right;
            }
            if (g_runtime_global_context.input_system->GetButton("Right")->GetAction() == InputAction::Press)
            {
                movement += right;
            }
            if (g_runtime_global_context.input_system->GetButton("Forward")->GetAction() == InputAction::Press)
            {
                movement += forward;
            }
            if (g_runtime_global_context.input_system->GetButton("Backward")->GetAction() == InputAction::Press)
            {
                movement += -forward;
            }
            if (g_runtime_global_context.input_system->GetButton("Up")->GetAction() == InputAction::Press)
            {
                movement += up;
            }
            if (g_runtime_global_context.input_system->GetButton("Down")->GetAction() == InputAction::Press)
            {
                movement += -up;
            }

            // TODO: config camera move velocity
            movement *= dt * 20.0f;

            transform_component->position += movement;
        }
    }
} // namespace Meow
