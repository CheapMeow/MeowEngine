#include "camera_3d_component.hpp"

#include "core/math/math.h"
#include "function/components/model/model_component.h"
#include "function/global/runtime_context.h"
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

        m_frustum.updatePlanes(transfrom_shared_ptr->position,
                               transfrom_shared_ptr->rotation,
                               field_of_view,
                               aspect_ratio,
                               near_plane,
                               far_plane);

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

        auto bounding = model_shared_ptr->model.lock()->GetBounding();
        bounding.min  = bounding.min * transform_shared_ptr->scale + transform_shared_ptr->position;
        bounding.max  = bounding.max * transform_shared_ptr->scale + transform_shared_ptr->position;

        return CheckVisibility(&bounding);
    }

    bool Camera3DComponent::CheckVisibility(BoundingBox* bounding) { return m_frustum.checkIfInside(bounding); }

    std::pair<glm::vec3, glm::quat> Camera3DComponent::CalculateFreeCameraDeltas(float dt)
    {
        // Default zero deltas
        glm::vec3 movement_delta(0.0f);
        glm::quat rotation_delta(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion

        if (!m_transform.lock())
        {
            return {movement_delta, rotation_delta};
        }
        
        // Handle mouse rotation
        if (g_runtime_context.input_system->GetButton("RightMouse")->GetAction() == InputAction::Press)
        {
            float dx = g_runtime_context.input_system->GetAxis("MouseX")->GetAmount();
            float dy = g_runtime_context.input_system->GetAxis("MouseY")->GetAmount();

            auto      transform_component = m_transform.lock();
            glm::vec3 temp_right          = transform_component->rotation * glm::vec3(1.0f, 0.0f, 0.0f);

            glm::quat dyaw =
                Math::QuaternionFromAngleAxis(-dx * dt * camera_rotate_velocity, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::quat dpitch = Math::QuaternionFromAngleAxis(-dy * dt * camera_rotate_velocity, temp_right);

            rotation_delta = dyaw * dpitch;
        }

        // Calculate movement direction vectors
        auto      transform_component = m_transform.lock();
        glm::vec3 right               = transform_component->rotation * glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 forward             = transform_component->rotation * glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 up                  = glm::vec3(0.0f, 1.0f, 0.0f);

        // Handle keyboard movement
        if (g_runtime_context.input_system->GetButton("Left")->GetAction() == InputAction::Press)
        {
            movement_delta += -right;
        }
        if (g_runtime_context.input_system->GetButton("Right")->GetAction() == InputAction::Press)
        {
            movement_delta += right;
        }
        if (g_runtime_context.input_system->GetButton("Forward")->GetAction() == InputAction::Press)
        {
            movement_delta += forward;
        }
        if (g_runtime_context.input_system->GetButton("Backward")->GetAction() == InputAction::Press)
        {
            movement_delta += -forward;
        }
        if (g_runtime_context.input_system->GetButton("Up")->GetAction() == InputAction::Press)
        {
            movement_delta += up;
        }
        if (g_runtime_context.input_system->GetButton("Down")->GetAction() == InputAction::Press)
        {
            movement_delta += -up;
        }

        // Normalize and scale movement
        if (glm::length(movement_delta) > 0.0f)
        {
            movement_delta = glm::normalize(movement_delta) * dt * camera_move_velocity;
        }

        return {movement_delta, rotation_delta};
    }

    void Camera3DComponent::ApplySmoothCameraMovement(float            dt,
                                                      const glm::vec3& movement_delta,
                                                      const glm::quat& rotation_delta)
    {
        if (std::shared_ptr<Transform3DComponent> transform_component = m_transform.lock())
        {
            // Calculate target values
            glm::quat target_rotation = rotation_delta * transform_component->rotation;
            glm::vec3 target_position = transform_component->position + movement_delta;

            // Apply smooth rotation (spherical interpolation)
            transform_component->rotation = glm::slerp(
                transform_component->rotation, target_rotation, 1.0f - std::exp(-rotation_smooth_factor * dt));

            // Apply smooth movement (linear interpolation)
            transform_component->position =
                glm::mix(transform_component->position, target_position, 1.0f - std::exp(-movement_smooth_factor * dt));
        }
    }

    void Camera3DComponent::TickFreeCamera(float dt)
    {
        auto [movement_delta, rotation_delta] = CalculateFreeCameraDeltas(dt);
        ApplySmoothCameraMovement(dt, movement_delta, rotation_delta);
    }
} // namespace Meow
