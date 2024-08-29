#include "camera_3d_component.hpp"

#include "core/math/math.h"
#include "function/components/model/model_component.h"
#include "function/global/runtime_global_context.h"
#include "function/object/game_object.h"

namespace Meow
{
    void Camera3DComponent::Start()
    {
        if (!m_parent_object.lock())
            MEOW_INFO("Not Found!");
        m_transform = m_parent_object.lock()->TryGetComponent<Transform3DComponent>("Transform3DComponent");
    }

    void Camera3DComponent::Tick(float dt)
    {
        auto transfrom_shared_ptr = m_transform.lock();

        glm::mat4 view = glm::mat4(1.0f);
        view           = glm::mat4_cast(glm::conjugate(transfrom_shared_ptr->rotation)) * view;
        view           = glm::translate(view, -transfrom_shared_ptr->position);

        m_frustum.updatePlanes(
            view, transfrom_shared_ptr->position, field_of_view, aspect_ratio, near_plane, far_plane);

        if (camera_mode == CameraMode::Free)
        {
            TickFreeCamera(dt);
        }
    }

    bool Camera3DComponent::FrustumCulling(std::shared_ptr<GameObject> gameobject)
    {
        auto transform_shared_ptr = gameobject->TryGetComponent<Transform3DComponent>("Transform3DComponent");
        if (!transform_shared_ptr)
            return false;

        auto model_shared_ptr = gameobject->TryGetComponent<ModelComponent>("ModelComponent");
        if (!model_shared_ptr)
            return false;

        auto bounding = model_shared_ptr->model_ptr.lock()->GetBounding();
        bounding.min  = bounding.min * transform_shared_ptr->scale + transform_shared_ptr->position;
        bounding.max  = bounding.max * transform_shared_ptr->scale + transform_shared_ptr->position;
        return CheckVisibility(&bounding);
    }

    bool Camera3DComponent::CheckVisibility(BoundingBox* bounding) { return m_frustum.checkIfInside(bounding); }

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
