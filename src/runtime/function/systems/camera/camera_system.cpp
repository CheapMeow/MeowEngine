#include "camera_system.h"

#include "core/log/log.h"
#include "core/math/math.h"
#include "function/global/runtime_global_context.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    void CameraSystem::Start() {}

    void CameraSystem::Update(float frame_time)
    {
        for (auto [entity, transfrom_component, camera_component] :
             g_runtime_global_context.registry.view<Transform3DComponent, Camera3DComponent>().each())
        {
            // TODO: camera type
            UpdateFreeCamera(transfrom_component, camera_component, frame_time);
        }
    }

    void CameraSystem::UpdateFreeCamera(Transform3DComponent& transform_component,
                                        Camera3DComponent&    camera_component,
                                        float                 frame_time)
    {
        if (g_runtime_global_context.input_system->GetButton("RightMouse")->GetAction() == InputAction::Press)
        {
            float dx = g_runtime_global_context.input_system->GetAxis("MouseX")->GetAmount();
            float dy = g_runtime_global_context.input_system->GetAxis("MouseY")->GetAmount();

            glm::vec3 temp_right = transform_component.rotation * glm::vec3(1.0f, 0.0f, 0.0f);

            // TODO: config camera rotate velocity
            glm::quat dyaw   = Math::QuaternionFromAngleAxis(-dx * frame_time * 100.0f, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat dpitch = Math::QuaternionFromAngleAxis(-dy * frame_time * 100.0f, temp_right);

            transform_component.rotation = dyaw * dpitch * transform_component.rotation;
        }

        glm::vec3 right   = transform_component.rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 forward = transform_component.rotation * glm::vec3(0.0f, 0.0f, 1.0f);
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
        movement *= frame_time * 20.0f;

        transform_component.position += movement;
    }
} // namespace Meow
