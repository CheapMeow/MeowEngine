#include "gizmo_widget.h"

#include "core/math/math.h"
#include "function/global/runtime_context.h"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include <ImGuizmo.h>

namespace Meow
{
    void GizmoWidget::ShowGameObjectGizmo(UUID id)
    {
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();

        if (g_runtime_context.input_system->GetButton("Q")->GetAction() == InputAction::Press)
        {
            m_GizmoType = -1;
        }
        if (g_runtime_context.input_system->GetButton("W")->GetAction() == InputAction::Press)
        {
            m_GizmoType = ImGuizmo::OPERATION::TRANSLATE;
        }
        if (g_runtime_context.input_system->GetButton("E")->GetAction() == InputAction::Press)
        {
            m_GizmoType = ImGuizmo::OPERATION::ROTATE;
        }
        if (g_runtime_context.input_system->GetButton("R")->GetAction() == InputAction::Press)
        {
            m_GizmoType = ImGuizmo::OPERATION::SCALE;
        }

        if (id != 0 && m_GizmoType != -1)
        {
            std::shared_ptr<Level> level = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

            const auto& all_gameobjects_map     = level->GetAllGameObjects();
            auto        current_gameobject_iter = all_gameobjects_map.find(id);
            if (current_gameobject_iter != all_gameobjects_map.end())
            {
                float windowWidth  = (float)ImGui::GetWindowWidth();
                float windowHeight = (float)ImGui::GetWindowHeight();
                ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

                if (!level)
                    MEOW_ERROR("shared ptr is invalid!");

                std::shared_ptr<GameObject> main_camera = level->GetGameObjectByID(level->GetMainCameraID()).lock();
                std::shared_ptr<Transform3DComponent> main_camera_transfrom_component =
                    main_camera->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                std::shared_ptr<Camera3DComponent> main_camera_component =
                    main_camera->TryGetComponent<Camera3DComponent>("Camera3DComponent");

                if (!main_camera)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!main_camera_transfrom_component)
                    MEOW_ERROR("shared ptr is invalid!");
                if (!main_camera_component)
                    MEOW_ERROR("shared ptr is invalid!");

                // Entity transform
                glm::ivec2 window_size = g_runtime_context.window_system->GetCurrentFocusWindow()->GetSize();
                glm::vec3  forward     = main_camera_transfrom_component->rotation * glm::vec3(0.0f, 0.0f, 1.0f);
                glm::mat4  view        = lookAt(main_camera_transfrom_component->position,
                                        main_camera_transfrom_component->position + forward,
                                        glm::vec3(0.0f, 1.0f, 0.0f));
                glm::mat4  projection =
                    Math::perspective_vk(main_camera_component->field_of_view,
                                         static_cast<float>(window_size[0]) / static_cast<float>(window_size[1]),
                                         main_camera_component->near_plane,
                                         main_camera_component->far_plane);

                auto gameobject_transform_component =
                    current_gameobject_iter->second->TryGetComponent<Transform3DComponent>("Transform3DComponent");
                if (!gameobject_transform_component)
                    MEOW_ERROR("shared ptr is invalid!");
                glm::mat4 gameobject_transform = gameobject_transform_component->GetTransform();

                // Snapping
                bool snap = g_runtime_context.input_system->GetButton("LeftControl")->GetAction() == InputAction::Press;
                float snapValue = 0.5f; // Snap to 0.5m for translation/scale
                // Snap to 45 degrees for rotation
                if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
                    snapValue = 45.0f;

                float snapValues[3] = {snapValue, snapValue, snapValue};

                ImGuizmo::Manipulate(glm::value_ptr(view),
                                     glm::value_ptr(projection),
                                     (ImGuizmo::OPERATION)m_GizmoType,
                                     ImGuizmo::LOCAL,
                                     glm::value_ptr(gameobject_transform),
                                     nullptr,
                                     snap ? snapValues : nullptr);

                if (ImGuizmo::IsUsing())
                {
                    glm::vec3 position, rotation, scale;
                    Math::DecomposeTransform(gameobject_transform, position, rotation, scale);

                    gameobject_transform_component->position = position;
                    gameobject_transform_component->rotation = rotation;
                    gameobject_transform_component->scale    = scale;
                }
            }
        }
    }
} // namespace Meow