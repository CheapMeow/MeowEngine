#include "camera_system.h"

#include "core/log/log.h"
#include "function/global/runtime_global_context.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    void CameraSystem::UpdateFreeCamera(Transform3DComponent& transform_component,
                                        Camera3DComponent&    camera_component,
                                        float                 frame_time)
    {
        if (g_runtime_global_context.input_system->GetButton("RightMouse")->GetAction() == InputAction::Press)
        {
            float dx = g_runtime_global_context.input_system->GetAxis("MouseX")->GetAmount();
            float dy = g_runtime_global_context.input_system->GetAxis("MouseY")->GetAmount();

            camera_component.yaw += -dx * frame_time * 5000.0f;
            if (camera_component.yaw > 360.0f)
            {
                camera_component.yaw -= 360.0f;
            }
            else if (camera_component.yaw < 0.0f)
            {
                camera_component.yaw += 360.0f;
            }
            camera_component.pitch = glm::clamp(camera_component.pitch + -dy * frame_time * 5000.0f, -89.0f, 89.0f);

            transform_component.rotation =
                glm::quat(glm::vec3(glm::radians(camera_component.pitch), glm::radians(camera_component.yaw), 0.0f));
        }

        glm::vec3 right   = transform_component.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 forward = transform_component.rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 up      = glm::vec3(0.0f, 1.0f, 0.0f);

        RUNTIME_INFO("right: ({}, {}, {})", right.x, right.y, right.z);
        RUNTIME_INFO("forward: ({}, {}, {})", forward.x, forward.y, forward.z);
        RUNTIME_INFO("frame_time: {}", frame_time);

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

        movement *= frame_time * 20.0f;

        transform_component.position += movement;
    }

    void CameraSystem::Update(float frame_time)
    {
        for (auto [entity, transfrom_component, camera_component] :
             g_runtime_global_context.registry.view<Transform3DComponent, Camera3DComponent>().each())
        {
            // TODO: camera type
            UpdateFreeCamera(transfrom_component, camera_component, frame_time);
        }
    }
} // namespace Meow
