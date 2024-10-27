#include "components_widget.h"

#include "pch.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <imgui.h>

namespace Meow
{
    ComponentsWidget::ComponentsWidget()
    {
        m_editor_ui_creator["TreeNodePush"] =
            [&](std::stack<bool>& node_states, const std::string& name, void* value_ptr) {
                ImGui::PushID(value_ptr);

                ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
                node_states.push(ImGui::TreeNodeEx(name.c_str(), flag));

                ImGui::PopID();
            };

        m_editor_ui_creator["TreeNodePop"] =
            [&](std::stack<bool>& node_states, const std::string& name, void* value_ptr) {
                if (node_states.empty())
                {
                    MEOW_ERROR("Tree node structure is wrong!");
                    return;
                }

                if (node_states.top())
                    ImGui::TreePop();
                node_states.pop();
            };

        m_editor_ui_creator["glm::vec3"] =
            [&](std::stack<bool>& node_states, const std::string& name, void* value_ptr) {
                if (node_states.empty())
                {
                    MEOW_ERROR("Tree node structure is wrong!");
                    return;
                }

                if (!node_states.top())
                    return;

                DrawVecControl(name, *static_cast<glm::vec3*>(value_ptr));
            };

        m_editor_ui_creator["glm::quat"] =
            [&](std::stack<bool>& node_states, const std::string& name, void* value_ptr) {
                if (node_states.empty())
                {
                    MEOW_ERROR("Tree node structure is wrong!");
                    return;
                }

                if (!node_states.top())
                    return;

                glm::quat& rotation = *static_cast<glm::quat*>(value_ptr);
                glm::vec3  euler    = glm::eulerAngles(rotation);
                glm::vec3  degrees_val;

                degrees_val.x = glm::degrees(euler.x); // pitch
                degrees_val.y = glm::degrees(euler.y); // roll
                degrees_val.z = glm::degrees(euler.z); // yaw

                DrawVecControl(name, degrees_val);

                euler.x = glm::radians(degrees_val.x);
                euler.y = glm::radians(degrees_val.y);
                euler.z = glm::radians(degrees_val.z);

                rotation = glm::quat(euler);
            };

        m_editor_ui_creator["bool"] = [&](std::stack<bool>& node_states, const std::string& name, void* value_ptr) {
            if (node_states.empty())
            {
                MEOW_ERROR("Tree node structure is wrong!");
                return;
            }

            if (!node_states.top())
                return;

            ImGui::PushID(value_ptr);

            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            ImGui::Checkbox(name.c_str(), static_cast<bool*>(value_ptr));

            ImGui::PopID();
        };

        m_editor_ui_creator["int"] = [&](std::stack<bool>& node_states, const std::string& name, void* value_ptr) {
            if (node_states.empty())
            {
                MEOW_ERROR("Tree node structure is wrong!");
                return;
            }

            if (!node_states.top())
                return;

            ImGui::PushID(value_ptr);

            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(-FLT_MIN); // disable showing label for input
            ImGui::InputInt(name.c_str(), static_cast<int*>(value_ptr));
            ImGui::PopItemWidth();

            ImGui::PopID();
        };

        m_editor_ui_creator["float"] = [&](std::stack<bool>& node_states, const std::string& name, void* value_ptr) {
            if (node_states.empty())
            {
                MEOW_ERROR("Tree node structure is wrong!");
                return;
            }

            if (!node_states.top())
                return;

            ImGui::PushID(value_ptr);

            ImGui::Text("%s", name.c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(-FLT_MIN); // disable showing label for input
            ImGui::InputFloat(name.c_str(), static_cast<float*>(value_ptr));
            ImGui::PopItemWidth();

            ImGui::PopID();
        };

        m_editor_ui_creator["std::string"] =
            [&](std::stack<bool>& node_states, const std::string& name, void* value_ptr) {
                if (node_states.empty())
                {
                    MEOW_ERROR("Tree node structure is wrong!");
                    return;
                }

                if (!node_states.top())
                    return;

                ImGui::PushID(value_ptr);

                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                ImGui::Text("%s", (*static_cast<std::string*>(value_ptr)).c_str());

                ImGui::PopID();
            };
    }

    void ComponentsWidget::CreateGameObjectUI(const std::shared_ptr<GameObject> go)
    {
        FUNCTION_TIMER();

        m_editor_ui_creator["TreeNodePush"](m_node_states, go->GetName(), nullptr);
        for (reflect::refl_shared_ptr<Component> comp_ptr : go->GetComponents())
        {
            CreateLeafNodeUI(comp_ptr);
        }
        m_editor_ui_creator["TreeNodePop"](m_node_states, go->GetName(), nullptr);
    }

    void ComponentsWidget::CreateLeafNodeUI(const reflect::refl_shared_ptr<Component> comp_ptr)
    {
        FUNCTION_TIMER();

        if (!reflect::Registry::instance().HasType(comp_ptr.type_name))
            return;

        const reflect::TypeDescriptor& type_desc = reflect::Registry::instance().GetType(comp_ptr.type_name);

        const std::vector<reflect::FieldAccessor>& field_accessors = type_desc.GetFields();
        for (const reflect::FieldAccessor& field_accessor : field_accessors)
        {
            if (m_editor_ui_creator.find(field_accessor.type_name()) != m_editor_ui_creator.end())
            {
                m_editor_ui_creator[field_accessor.type_name()](
                    m_node_states, field_accessor.name(), field_accessor.get(comp_ptr.shared_ptr.get()));
            }
        }

        const std::vector<reflect::ArrayAccessor>& array_accessors = type_desc.GetArrays();
        for (const reflect::ArrayAccessor& array_accessor : array_accessors)
        {
            if (m_editor_ui_creator.find(array_accessor.inner_type_name()) == m_editor_ui_creator.end())
                continue;

            std::size_t array_size = array_accessor.get_size(comp_ptr.shared_ptr.get());
            for (std::size_t i = 0; i < array_size; i++)
            {
                m_editor_ui_creator[array_accessor.inner_type_name()](
                    m_node_states, array_accessor.inner_type_name(), array_accessor.get(comp_ptr.shared_ptr.get(), i));
            }
        }
    }

    void
    ComponentsWidget::DrawVecControl(const std::string& label, glm::vec3& values, float reset_value, float column_width)
    {
        FUNCTION_TIMER();

        ImGui::PushID(&values);

        ImGui::Columns(2);
        ImGui::SetColumnWidth(0, column_width);
        ImGui::Text("%s", label.c_str());
        ImGui::NextColumn();

        // Calculate width for each item manually
        float itemWidth = ImGui::CalcItemWidth() / 3.0f;

        ImGui::PushItemWidth(itemWidth);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2 {0, 0});

        float  lineHeight = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0f;
        ImVec2 buttonSize = {lineHeight + 3.0f, lineHeight};

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.9f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.8f, 0.1f, 0.15f, 1.0f});
        if (ImGui::Button("X", buttonSize))
            values.x = reset_value;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.3f, 0.55f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.2f, 0.45f, 0.2f, 1.0f});
        if (ImGui::Button("Y", buttonSize))
            values.y = reset_value;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4 {0.2f, 0.35f, 0.9f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4 {0.1f, 0.25f, 0.8f, 1.0f});
        if (ImGui::Button("Z", buttonSize))
            values.z = reset_value;
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");

        ImGui::PopItemWidth();
        ImGui::PopStyleVar();

        ImGui::Columns(1);
        ImGui::PopID();
    }
} // namespace Meow