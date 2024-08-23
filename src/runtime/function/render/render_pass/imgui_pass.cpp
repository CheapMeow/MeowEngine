#include "imgui_pass.h"

#include "pch.h"

#include "function/global/runtime_global_context.h"
#include "function/render/utils/flame_graph_drawer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

namespace Meow
{
    ImguiPass::ImguiPass(std::nullptr_t)
        : RenderPass(nullptr)
    {
        m_editor_ui_creator["TreeNodePush"] = [&](const std::string& name, void* value_ptr) {
            ImGuiTreeNodeFlags flag = ImGuiTreeNodeFlags_DefaultOpen;
            m_tree_node_open_states.push(ImGui::TreeNodeEx(name.c_str(), flag));
        };

        m_editor_ui_creator["TreeNodePop"] = [&](const std::string& name, void* value_ptr) {
            if (m_tree_node_open_states.top())
                ImGui::TreePop();
            m_tree_node_open_states.pop();
        };

        m_editor_ui_creator["glm::vec3"] = [&](const std::string& name, void* value_ptr) {
            if (!m_tree_node_open_states.top())
                return;

            DrawVecControl(name, *static_cast<glm::vec3*>(value_ptr));
        };

        m_editor_ui_creator["glm::quat"] = [&](const std::string& name, void* value_ptr) {
            if (!m_tree_node_open_states.empty() && !m_tree_node_open_states.top())
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

        m_editor_ui_creator["bool"] = [&](const std::string& name, void* value_ptr) {
            if (!m_tree_node_open_states.empty() && !m_tree_node_open_states.top())
                return;

            ImGui::Text("%s", name.c_str());
            ImGui::Checkbox(name.c_str(), static_cast<bool*>(value_ptr));
        };

        m_editor_ui_creator["int"] = [&](const std::string& name, void* value_ptr) {
            if (!m_tree_node_open_states.empty() && !m_tree_node_open_states.top())
                return;

            ImGui::Text("%s", name.c_str());
            ImGui::InputInt(name.c_str(), static_cast<int*>(value_ptr));
        };

        m_editor_ui_creator["float"] = [&](const std::string& name, void* value_ptr) {
            if (!m_tree_node_open_states.empty() && !m_tree_node_open_states.top())
                return;

            ImGui::Text("%s", name.c_str());
            ImGui::InputFloat(name.c_str(), static_cast<float*>(value_ptr));
        };

        m_editor_ui_creator["std::string"] = [&](const std::string& name, void* value_ptr) {
            if (!m_tree_node_open_states.empty() && !m_tree_node_open_states.top())
                return;

            ImGui::Text("%s", name.c_str());
            ImGui::Text(name.c_str(), (*static_cast<std::string*>(value_ptr)).c_str());
        };
    }

    ImguiPass::ImguiPass(vk::raii::PhysicalDevice const& physical_device,
                         vk::raii::Device const&         device,
                         SurfaceData&                    surface_data,
                         vk::raii::CommandPool const&    command_pool,
                         vk::raii::Queue const&          queue,
                         DescriptorAllocatorGrowable&    m_descriptor_allocator)
        : RenderPass(nullptr)
    {
        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        vk::AttachmentReference swapchain_attachment_reference(0, vk::ImageLayout::eColorAttachmentOptimal);

        // swap chain attachment
        vk::AttachmentDescription attachment_description(vk::AttachmentDescriptionFlags(), /* flags */
                                                         color_format,                     /* format */
                                                         vk::SampleCountFlagBits::e1,      /* samples */
                                                         vk::AttachmentLoadOp::eLoad,      /* loadOp */
                                                         vk::AttachmentStoreOp::eStore,    /* storeOp */
                                                         vk::AttachmentLoadOp::eDontCare,  /* stencilLoadOp */
                                                         vk::AttachmentStoreOp::eDontCare, /* stencilStoreOp */
                                                         vk::ImageLayout::ePresentSrcKHR,  /* initialLayout */
                                                         vk::ImageLayout::ePresentSrcKHR); /* finalLayout */

        vk::SubpassDescription subpass_description(
            vk::SubpassDescription(vk::SubpassDescriptionFlags(),    /* flags */
                                   vk::PipelineBindPoint::eGraphics, /* pipelineBindPoint */
                                   {},                               /* pInputAttachments */
                                   swapchain_attachment_reference,   /* pColorAttachments */
                                   {},                               /* pResolveAttachments */
                                   {},                               /* pDepthStencilAttachment */
                                   nullptr));                        /* pPreserveAttachments */

        vk::SubpassDependency dependencies(VK_SUBPASS_EXTERNAL,                               /* srcSubpass */
                                           0,                                                 /* dstSubpass */
                                           vk::PipelineStageFlagBits::eColorAttachmentOutput, /* srcStageMask */
                                           vk::PipelineStageFlagBits::eColorAttachmentOutput, /* dstStageMask */
                                           {},                                                /* srcAccessMask */
                                           vk::AccessFlagBits::eColorAttachmentWrite,         /* dstAccessMask */
                                           {});                                               /* dependencyFlags */

        vk::RenderPassCreateInfo render_pass_create_info(vk::RenderPassCreateFlags(), /* flags */
                                                         attachment_description,      /* pAttachments */
                                                         subpass_description,         /* pSubpasses */
                                                         dependencies);               /* pDependencies */

        render_pass = vk::raii::RenderPass(device, render_pass_create_info);

        // loadOp is load, clear value doesn't matter
        clear_values.resize(1);
        clear_values[0].color = vk::ClearColorValue(0.6f, 0.6f, 0.6f, 1.0f);
    }

    void ImguiPass::RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                        vk::raii::Device const&                 device,
                                        vk::raii::CommandBuffer const&          command_buffer,
                                        SurfaceData&                            surface_data,
                                        std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                        vk::Extent2D const&                     extent)
    {
        // clear

        framebuffers.clear();

        // Provide attachment information to frame buffer

        vk::ImageView attachments[1];

        vk::FramebufferCreateInfo framebuffer_create_info(vk::FramebufferCreateFlags(), /* flags */
                                                          *render_pass,                 /* renderPass */
                                                          1,                            /* attachmentCount */
                                                          attachments,                  /* pAttachments */
                                                          extent.width,                 /* width */
                                                          extent.height,                /* height */
                                                          1);                           /* layers */

        framebuffers.reserve(swapchain_image_views.size());
        for (auto const& imageView : swapchain_image_views)
        {
            attachments[0] = *imageView;
            framebuffers.push_back(vk::raii::Framebuffer(device, framebuffer_create_info));
        }
    }

    void ImguiPass::Start(vk::raii::CommandBuffer const& command_buffer,
                          Meow::SurfaceData const&       surface_data,
                          uint32_t                       current_image_index)
    {
        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Meow Engine");

        ImGui::Combo("Current Render Pass",
                     &g_runtime_global_context.render_system->cur_render_pass,
                     g_runtime_global_context.render_system->render_pass_names.data(),
                     g_runtime_global_context.render_system->render_pass_names.size());

        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("invocationCount_fs = %d", invocationCount_fs_query_count);

        std::shared_ptr<Level> level_ptr = g_runtime_global_context.level_system->GetCurrentActiveLevel().lock();

#ifdef MEOW_DEBUG
        if (!level_ptr)
            RUNTIME_ERROR("shared ptr is invalid!");
#endif

        const auto& all_gameobjects_map = level_ptr->GetAllGameObjects();
        for (const auto& kv : all_gameobjects_map)
        {
            CreateGameObjectUI(kv.second);
        }

        FlameGraphDrawer::Draw(TimerSingleton::Get().GetScopeTimes(),
                               TimerSingleton::Get().GetMaxDepth(),
                               TimerSingleton::Get().GetGlobalStart());

        ImGui::End();

        RenderPass::Start(command_buffer, surface_data, current_image_index);
    }

    void ImguiPass::Draw(vk::raii::CommandBuffer const& command_buffer)
    {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *command_buffer);

        // Specially for docking branch
        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void ImguiPass::CreateGameObjectUI(const std::shared_ptr<GameObject> go)
    {
        m_editor_ui_creator["TreeNodePush"](go->GetName(), nullptr);
        for (reflect::refl_shared_ptr<Component> comp_ptr : go->GetComponents())
        {
            CreateLeafNodeUI(comp_ptr);
        }
        m_editor_ui_creator["TreeNodePop"](go->GetName(), nullptr);
    }

    void ImguiPass::CreateLeafNodeUI(const reflect::refl_shared_ptr<Component> comp_ptr)
    {
        if (!reflect::Registry::instance().HasType(comp_ptr.type_name))
            return;

        const reflect::TypeDescriptor& type_desc = reflect::Registry::instance().GetType(comp_ptr.type_name);
        const std::vector<reflect::FieldAccessor>& field_accessors = type_desc.GetFields();

        for (const reflect::FieldAccessor& field_accessor : field_accessors)
        {
            if (m_editor_ui_creator.find(field_accessor.type_name()) != m_editor_ui_creator.end())
            {
                m_editor_ui_creator[field_accessor.type_name()](field_accessor.name(),
                                                                field_accessor.get(comp_ptr.shared_ptr.get()));
            }
        }
    }

    void ImguiPass::DrawVecControl(const std::string& label, glm::vec3& values, float reset_value, float column_width)
    {
        ImGui::PushID(label.c_str());

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
