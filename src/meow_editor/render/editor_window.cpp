#include "editor_window.h"

#include "function/components/light/directional_light_component.h"
#include "global/editor_context.h"
#include "meow_runtime/core/reflect/reflect.hpp"
#include "meow_runtime/function/components/camera/camera_3d_component.hpp"
#include "meow_runtime/function/components/model/model_component.h"
#include "meow_runtime/function/components/transform/transform_3d_component.hpp"
#include "meow_runtime/function/global/runtime_context.h"
#include "meow_runtime/function/level/level.h"
#include "meow_runtime/function/render/geometry/geometry_factory.h"
#include "meow_runtime/function/render/utils/vulkan_initialization_utils.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <imgui.h>

namespace Meow
{
    EditorWindow::EditorWindow(std::size_t id, GLFWwindow* glfw_window)
        : GraphicsWindow(id, glfw_window)
    {
        CreateSurface();
        CreateSwapChian();
        CreatePerFrameData();
        CreateRenderPass();
        InitImGui();
        BindImageToImguiPass();

        OnSize().connect([&](glm::ivec2 new_size) { m_framebuffer_resized = true; });

        OnIconify().connect([&](bool iconified) { m_iconified = iconified; });

        auto& per_frame_data = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer     = per_frame_data.command_buffer;

        std::shared_ptr<Level> level = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

        if (!level)
            MEOW_ERROR("shared ptr is invalid!");

        {
            auto uuid = level->CreateObject();
            level->SetMainCameraID(uuid);
            std::shared_ptr<GameObject> current_gameobject = level->GetGameObjectByID(uuid).lock();

            if (!current_gameobject)
                MEOW_ERROR("GameObject is invalid!");

            current_gameobject->SetName("Camera");
            std::shared_ptr<Transform3DComponent> transform_ptr =
                TryAddComponent(current_gameobject, "Transform3DComponent", std::make_shared<Transform3DComponent>());
            std::shared_ptr<Camera3DComponent> camera_ptr =
                TryAddComponent(current_gameobject, "Camera3DComponent", std::make_shared<Camera3DComponent>());

            if (!transform_ptr)
                MEOW_ERROR("shared ptr is invalid!");
            if (!camera_ptr)
                MEOW_ERROR("shared ptr is invalid!");

            transform_ptr->position = glm::vec3(0.0f, 10.0f, -6.0f);
            transform_ptr->rotation = glm::quat(glm::vec3(-100.0f, 0.0f, 0.0f));

            camera_ptr->camera_mode  = CameraMode::Free;
            camera_ptr->aspect_ratio = (float)m_surface_data.extent.width / m_surface_data.extent.height;
        }

        {
            auto                        uuid               = level->CreateObject();
            std::shared_ptr<GameObject> current_gameobject = level->GetGameObjectByID(uuid).lock();
            TryAddComponent(
                current_gameobject, "DirectionalLightComponent", std::make_shared<DirectionalLightComponent>());
            auto directional_light_transform =
                TryAddComponent(current_gameobject, "Transform3DComponent", std::make_shared<Transform3DComponent>());
            directional_light_transform->position = glm::vec3(0.0f, 30.0f, -50.0f);
            directional_light_transform->rotation = glm::quat(glm::vec3(-100.0f, 0.0f, 0.0f));
        }

        GeometryFactory geometry_factory;
        geometry_factory.SetSphere(32, 32);

        {
            std::size_t        row_number    = 7;
            std::size_t        column_number = 7;
            float              spacing       = 2.5;
            std::vector<float> sphere_vertices =
                geometry_factory.GetVertices(m_render_pass_ptr->input_vertex_attributes);
            std::vector<uint32_t> sphere_indices = geometry_factory.GetIndices();
            for (std::size_t row = 0; row < row_number; ++row)
            {
                for (std::size_t col = 0; col < column_number; ++col)
                {
                    UUID                        uuid               = level->CreateObject();
                    std::shared_ptr<GameObject> current_gameobject = level->GetGameObjectByID(uuid).lock();

                    opaque_objects.push_back(current_gameobject);

                    if (!current_gameobject)
                        MEOW_ERROR("GameObject is invalid!");

                    current_gameobject->SetName("Sphere " + std::to_string(row * column_number + col));
                    auto transform_ptr = TryAddComponent(
                        current_gameobject, "Transform3DComponent", std::make_shared<Transform3DComponent>());
                    transform_ptr->position = glm::vec3((float)col * spacing - (float)column_number / 2.0f * spacing,
                                                        (float)row * spacing - (float)row_number / 2.0f * spacing,
                                                        0.0f);

                    auto current_gameobject_model_component =
                        TryAddComponent(current_gameobject, "ModelComponent", std::make_shared<ModelComponent>());
                    auto model_shared_ptr = std::make_shared<Model>(
                        sphere_vertices, sphere_indices, m_render_pass_ptr->input_vertex_attributes);

                    // TODO: hard code render pass cast
                    current_gameobject_model_component->material_id = m_forward_pass.GetForwardMatID();

                    if (model_shared_ptr)
                    {
                        g_runtime_context.resource_system->Register(model_shared_ptr);
                        current_gameobject_model_component->model = model_shared_ptr;
                    }
                }
            }
        }

        {
            std::size_t        row_number    = 7;
            std::size_t        column_number = 7;
            float              spacing       = 2.5;
            std::vector<float> sphere_vertices =
                geometry_factory.GetVertices(m_render_pass_ptr->input_vertex_attributes);
            std::vector<uint32_t> sphere_indices = geometry_factory.GetIndices();
            for (std::size_t row = 0; row < row_number; ++row)
            {
                for (std::size_t col = 0; col < column_number; ++col)
                {
                    UUID                        uuid               = level->CreateObject();
                    std::shared_ptr<GameObject> current_gameobject = level->GetGameObjectByID(uuid).lock();

                    translucent_objects.push_back(current_gameobject);

                    if (!current_gameobject)
                        MEOW_ERROR("GameObject is invalid!");

                    current_gameobject->SetName("Sphere Translucent" + std::to_string(row * column_number + col));
                    auto transform_ptr = TryAddComponent(
                        current_gameobject, "Transform3DComponent", std::make_shared<Transform3DComponent>());
                    transform_ptr->position = glm::vec3((float)col * spacing - (float)column_number / 2.0f * spacing,
                                                        (float)row * spacing - (float)row_number / 2.0f * spacing,
                                                        -5.0f);

                    auto current_gameobject_model_component =
                        TryAddComponent(current_gameobject, "ModelComponent", std::make_shared<ModelComponent>());
                    auto model_shared_ptr = std::make_shared<Model>(
                        sphere_vertices, sphere_indices, m_render_pass_ptr->input_vertex_attributes);

                    // TODO: hard code render pass cast
                    current_gameobject_model_component->material_id = m_forward_pass.GetTranslucentMatID();

                    if (model_shared_ptr)
                    {
                        g_runtime_context.resource_system->Register(model_shared_ptr);
                        current_gameobject_model_component->model = model_shared_ptr;
                    }
                }
            }
        }

        geometry_factory.SetCube();

        {
            std::vector<float> cube_vertices = geometry_factory.GetVertices(m_render_pass_ptr->input_vertex_attributes);
            std::vector<uint32_t> cube_indices = geometry_factory.GetIndices();

            UUID                        uuid               = level->CreateObject();
            std::shared_ptr<GameObject> current_gameobject = level->GetGameObjectByID(uuid).lock();

            opaque_objects.push_back(current_gameobject);

            if (!current_gameobject)
                MEOW_ERROR("GameObject is invalid!");

            current_gameobject->SetName("Cube");
            auto transform_ptr =
                TryAddComponent(current_gameobject, "Transform3DComponent", std::make_shared<Transform3DComponent>());
            transform_ptr->position = glm::vec3(0.0f, 0.0f, 10.0f);

            auto current_gameobject_model_component =
                TryAddComponent(current_gameobject, "ModelComponent", std::make_shared<ModelComponent>());
            auto model_shared_ptr =
                std::make_shared<Model>(cube_vertices, cube_indices, m_render_pass_ptr->input_vertex_attributes);

            // TODO: hard code render pass cast
            current_gameobject_model_component->material_id = m_forward_pass.GetForwardMatID();

            {
                const reflect::TypeDescriptor& type_desc = reflect::Registry::instance().GetType("ModelComponent");
                const std::vector<reflect::MethodAccessor>& method_accessor = type_desc.GetMethods();
                for (const auto& method : method_accessor)
                {
                    std::cout << method.name() << std::endl;
                    method.Invoke(*current_gameobject_model_component.get());
                }
            }

            Component* component_ptr = current_gameobject_model_component.get();

            {
                const reflect::TypeDescriptor& type_desc = reflect::Registry::instance().GetType("Component");
                const std::vector<reflect::MethodAccessor>& method_accessor = type_desc.GetMethods();
                for (const auto& method : method_accessor)
                {
                    std::cout << method.name() << std::endl;
                    method.Invoke(*component_ptr);
                }
            }

            if (model_shared_ptr)
            {
                g_runtime_context.resource_system->Register(model_shared_ptr);
                current_gameobject_model_component->model = model_shared_ptr;
            }
        }

        geometry_factory.SetPlane();

        {
            std::vector<float> plane_vertices =
                geometry_factory.GetVertices(m_render_pass_ptr->input_vertex_attributes);
            std::vector<uint32_t> plane_indices = geometry_factory.GetIndices();

            UUID                        uuid               = level->CreateObject();
            std::shared_ptr<GameObject> current_gameobject = level->GetGameObjectByID(uuid).lock();

            opaque_objects.push_back(current_gameobject);

            if (!current_gameobject)
                MEOW_ERROR("GameObject is invalid!");

            current_gameobject->SetName("Plane");
            auto transform_ptr =
                TryAddComponent(current_gameobject, "Transform3DComponent", std::make_shared<Transform3DComponent>());
            transform_ptr->position = glm::vec3(0.0f, -10.0f, 10.0f);
            transform_ptr->scale    = glm::vec3(20.0f, 20.0f, 20.0f);

            auto current_gameobject_model_component =
                TryAddComponent(current_gameobject, "ModelComponent", std::make_shared<ModelComponent>());
            auto model_shared_ptr =
                std::make_shared<Model>(plane_vertices, plane_indices, m_render_pass_ptr->input_vertex_attributes);

            // TODO: hard code render pass cast
            current_gameobject_model_component->material_id = m_forward_pass.GetForwardMatID();

            if (model_shared_ptr)
            {
                g_runtime_context.resource_system->Register(model_shared_ptr);
                current_gameobject_model_component->model = model_shared_ptr;
            }
        }
    }

    EditorWindow::~EditorWindow()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();
        logical_device.waitIdle();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_imgui_pass            = nullptr;
        m_imgui_descriptor_pool = nullptr;

        m_per_frame_data.clear();
        m_depth_to_color_pass        = nullptr;
        m_shadow_coord_to_color_pass = nullptr;
        m_shadow_map_pass            = nullptr;
        m_forward_pass               = nullptr;
        m_deferred_pass              = nullptr;
        m_swapchain_data             = nullptr;
        m_surface_data               = nullptr;
    }

    void EditorWindow::Tick(float dt)
    {
        FUNCTION_TIMER();

        m_wait_until_next_tick_signal();
        m_wait_until_next_tick_signal.clear();

        if (m_iconified)
            return;

        const vk::raii::Device& logical_device            = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::Queue&  graphics_queue            = g_runtime_context.render_system->GetGraphicsQueue();
        const vk::raii::Queue&  present_queue             = g_runtime_context.render_system->GetPresentQueue();
        auto&                   per_frame_data            = m_per_frame_data[m_current_frame_index];
        auto&                   cmd_buffer                = per_frame_data.command_buffer;
        auto&                   image_acquired_semaphore  = per_frame_data.image_acquired_semaphore;
        auto&                   render_finished_semaphore = per_frame_data.render_finished_semaphore;
        auto&                   in_flight_fence           = per_frame_data.in_flight_fence;

        m_shadow_map_pass.UpdateUniformBuffer();
        m_shadow_coord_to_color_pass.UpdateUniformBuffer();
        m_render_pass_ptr->UpdateUniformBuffer();
        m_forward_pass.PopulateDirectionalLightData(m_shadow_map_pass.GetShadowMap());

        // ------------------- render -------------------

        auto [result, m_current_image_index] =
            SwapchainNextImageWrapper(m_swapchain_data.swap_chain, k_fence_timeout, *image_acquired_semaphore);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_framebuffer_resized)
        {
            m_framebuffer_resized = false;
            RecreateSwapChain();
            return;
        }
        assert(result == vk::Result::eSuccess);
        assert(m_current_image_index < m_swapchain_data.images.size());

        cmd_buffer.begin({});

        m_shadow_map_pass.Start(cmd_buffer, m_surface_data.extent, 0);
        m_shadow_map_pass.Draw(cmd_buffer);
        m_shadow_map_pass.End(cmd_buffer);

        m_depth_to_color_pass.Start(cmd_buffer, m_surface_data.extent, 0);
        m_depth_to_color_pass.Draw(cmd_buffer);
        m_depth_to_color_pass.End(cmd_buffer);

        m_shadow_coord_to_color_pass.Start(cmd_buffer, m_surface_data.extent, 0);
        m_shadow_coord_to_color_pass.Draw(cmd_buffer);
        m_shadow_coord_to_color_pass.End(cmd_buffer);

        m_render_pass_ptr->Start(cmd_buffer, m_surface_data.extent, 0);
        m_render_pass_ptr->Draw(cmd_buffer);
        m_render_pass_ptr->End(cmd_buffer);

        m_imgui_pass.Start(cmd_buffer, m_surface_data.extent, m_current_image_index);
        m_imgui_pass.Draw(cmd_buffer);
        m_imgui_pass.End(cmd_buffer);

        cmd_buffer.end();

        vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo         submit_info(
            *image_acquired_semaphore, wait_destination_stage_mask, *cmd_buffer, *render_finished_semaphore);
        graphics_queue.submit(submit_info, *in_flight_fence);

        while (vk::Result::eTimeout == logical_device.waitForFences({*in_flight_fence}, VK_TRUE, k_fence_timeout))
            ;
        cmd_buffer.reset();
        logical_device.resetFences({*in_flight_fence});

        vk::PresentInfoKHR present_info(
            *render_finished_semaphore, *m_swapchain_data.swap_chain, m_current_image_index);
        result = QueuePresentWrapper(present_queue, present_info);
        switch (result)
        {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                MEOW_ERROR("vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n");
                break;
            default:
                assert(false); // an unexpected result is returned !
        }

        m_current_frame_index = (m_current_frame_index + 1) % k_max_frames_in_flight;

        m_render_pass_ptr->AfterPresent();
        m_imgui_pass.AfterPresent();

        Window::Tick(dt);
    }

    void EditorWindow::CreateRenderPass()
    {
        m_shadow_map_pass            = ShadowMapPass(m_surface_data);
        m_depth_to_color_pass        = DepthToColorPass(m_surface_data);
        m_shadow_coord_to_color_pass = ShadowCoordToColorPass(m_surface_data);
        m_deferred_pass              = DeferredPassEditor(m_surface_data);
        m_forward_pass               = ForwardPassEditor(m_surface_data);
        m_imgui_pass                 = ImGuiPass(m_surface_data);

        RefreshFrameBuffers();

        m_imgui_pass.OnMSAAEnabledChanged().connect([&](bool enabled) {
            m_wait_until_next_tick_signal.connect([&, enabled]() {
                const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();
                logical_device.waitIdle();

                m_forward_pass.SetMSAAEnabled(enabled);
                m_forward_pass.CreateRenderPass();
                m_forward_pass.CreateMaterial();

                m_deferred_pass.RefreshFrameBuffers({*m_offscreen_render_target->image_view}, m_surface_data.extent);
                m_forward_pass.RefreshFrameBuffers({*m_offscreen_render_target->image_view}, m_surface_data.extent);
            });
        });

        m_imgui_pass.OnPassChanged().connect([&](int cur_render_pass) {
            // switch render pass
            if (cur_render_pass == 0)
                m_render_pass_ptr = &m_deferred_pass;
            else if (cur_render_pass == 1)
                m_render_pass_ptr = &m_forward_pass;

            g_editor_context.profile_system->ClearProfile();

            for (int i = 0; i < opaque_objects.size(); i++)
            {
                auto opaque_object_ptr = opaque_objects[i].lock();
                if (opaque_object_ptr)
                {
                    auto current_gameobject_model_component =
                        opaque_object_ptr->TryGetComponent<ModelComponent>("ModelComponent");

                    if (cur_render_pass == 1)
                        current_gameobject_model_component->material_id = m_forward_pass.GetForwardMatID();
                    else if (cur_render_pass == 0)
                        current_gameobject_model_component->material_id = m_deferred_pass.GetObj2AttachmentMatID();
                }
            }
        });

        m_render_pass_ptr = &m_forward_pass;

        m_depth_to_color_pass.BindShadowMap(m_shadow_map_pass.GetShadowMap());
        m_forward_pass.BindShadowMap(m_shadow_map_pass.GetShadowMap());
        m_forward_pass.PopulateDirectionalLightData(m_shadow_map_pass.GetShadowMap());
    }

    void EditorWindow::InitImGui()
    {
        const vk::raii::Instance&       vulkan_instance = g_runtime_context.render_system->GetInstance();
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const auto graphics_queue_family_index = g_runtime_context.render_system->GetGraphicsQueueFamiliyIndex();
        const vk::raii::Queue& graphics_queue  = g_runtime_context.render_system->GetGraphicsQueue();

        std::vector<vk::DescriptorPoolSize> pool_sizes = {{vk::DescriptorType::eSampler, 1000},
                                                          {vk::DescriptorType::eCombinedImageSampler, 1000},
                                                          {vk::DescriptorType::eSampledImage, 1000},
                                                          {vk::DescriptorType::eStorageImage, 1000},
                                                          {vk::DescriptorType::eUniformTexelBuffer, 1000},
                                                          {vk::DescriptorType::eStorageTexelBuffer, 1000},
                                                          {vk::DescriptorType::eUniformBuffer, 1000},
                                                          {vk::DescriptorType::eStorageBuffer, 1000},
                                                          {vk::DescriptorType::eUniformBufferDynamic, 1000},
                                                          {vk::DescriptorType::eStorageBufferDynamic, 1000},
                                                          {vk::DescriptorType::eInputAttachment, 1000}};
        vk::DescriptorPoolCreateInfo        descriptor_pool_create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1000, pool_sizes);
        m_imgui_descriptor_pool = vk::raii::DescriptorPool(logical_device, descriptor_pool_create_info);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;   // Enable Multi-Viewport / Platform Windows
        // io.ConfigViewportsNoAutoMerge = true;
        // io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsLight();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular
        // ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding              = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends

        // Because in ImGui docking branch, each viewport has its own `render pass`, but they doesn't have their
        // own `pipeline`. They use the `pipeline` created from the `render pass` pass in `ImGui_ImplVulkan_Init`. So
        // the `render pass` pass in `ImGui_ImplVulkan_Init` should be compatiable with the `render pass` used in other
        // viewports. There is only one way to do that: use seperate render pass while make this render pass compatiable
        // with those in viewports

        ImGui_ImplGlfw_InitForVulkan(m_glfw_window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = *vulkan_instance;
        init_info.PhysicalDevice            = *physical_device;
        init_info.Device                    = *logical_device;
        init_info.QueueFamily               = graphics_queue_family_index;
        init_info.Queue                     = *graphics_queue;
        init_info.DescriptorPool            = *m_imgui_descriptor_pool;
        init_info.Subpass                   = 0;
        init_info.MinImageCount             = k_max_frames_in_flight;
        init_info.ImageCount                = k_max_frames_in_flight;
        init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        init_info.RenderPass                = *m_imgui_pass.render_pass;

        ImGui_ImplVulkan_Init(&init_info);

        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [](vk::raii::CommandBuffer& command_buffer) { ImGui_ImplVulkan_CreateFontsTexture(); });
    }

    void EditorWindow::RecreateSwapChain()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        logical_device.waitIdle();

        m_per_frame_data.clear();
        m_swapchain_data = nullptr;
        m_surface_data   = nullptr;

        CreateSurface();
        CreateSwapChian();
        CreatePerFrameData();
        RefreshFrameBuffers();
        BindImageToImguiPass();

        // update aspect ratio

        std::shared_ptr<Level>      level              = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        std::shared_ptr<GameObject> current_gameobject = level->GetGameObjectByID(level->GetMainCameraID()).lock();

        if (!current_gameobject)
            return;

        std::shared_ptr<Camera3DComponent> camera_ptr =
            current_gameobject->TryGetComponent<Camera3DComponent>("Camera3DComponent");

        camera_ptr->aspect_ratio = (float)m_surface_data.extent.width / m_surface_data.extent.height;
    }

    void EditorWindow::RefreshFrameBuffers()
    {
        std::vector<vk::ImageView> swapchain_image_views;
        swapchain_image_views.resize(m_swapchain_data.image_views.size());
        for (int i = 0; i < m_swapchain_data.image_views.size(); i++)
        {
            swapchain_image_views[i] = *m_swapchain_data.image_views[i];
        }

        m_offscreen_render_target = ImageData::CreateRenderTarget(m_color_format,
                                                                  m_surface_data.extent,
                                                                  vk::ImageUsageFlagBits::eColorAttachment |
                                                                      vk::ImageUsageFlagBits::eInputAttachment,
                                                                  vk::ImageAspectFlagBits::eColor,
                                                                  {},
                                                                  false);

        m_depth_debugging_attachment = ImageData::CreateAttachment(m_depth_format,
                                                                   m_surface_data.extent,
                                                                   vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                                   vk::ImageAspectFlagBits::eDepth,
                                                                   {},
                                                                   false);

        m_offscreen_render_target->SetDebugName("Offscreen Render Target");
        m_depth_debugging_attachment->SetDebugName("Depth Debugging Attachment");

        m_shadow_coord_to_color_pass.BindDepthAttachment(m_depth_debugging_attachment);

        m_shadow_map_pass.RefreshFrameBuffers({*m_offscreen_render_target->image_view}, m_surface_data.extent);
        m_depth_to_color_pass.RefreshFrameBuffers({*m_offscreen_render_target->image_view}, m_surface_data.extent);
        m_shadow_coord_to_color_pass.RefreshFrameBuffers({*m_offscreen_render_target->image_view},
                                                         m_surface_data.extent);
        m_deferred_pass.RefreshFrameBuffers({*m_offscreen_render_target->image_view}, m_surface_data.extent);
        m_forward_pass.RefreshFrameBuffers({*m_offscreen_render_target->image_view}, m_surface_data.extent);
        m_imgui_pass.RefreshFrameBuffers(swapchain_image_views, m_surface_data.extent);
    }

    void EditorWindow::BindImageToImguiPass()
    {
        m_imgui_pass.RefreshOffscreenRenderTarget(*m_offscreen_render_target->sampler,
                                                  *m_offscreen_render_target->image_view,
                                                  static_cast<VkImageLayout>(m_offscreen_render_target->layout));

        m_imgui_pass.RefreshShadowMap(
            *m_depth_to_color_pass.GetDepthToColorRenderTarget()->sampler,
            *m_depth_to_color_pass.GetDepthToColorRenderTarget()->image_view,
            static_cast<VkImageLayout>(m_depth_to_color_pass.GetDepthToColorRenderTarget()->layout));

        m_imgui_pass.RefreshShadowCoord(
            *m_shadow_coord_to_color_pass.GetShadowCoordToColorRenderTarget()->sampler,
            *m_shadow_coord_to_color_pass.GetShadowCoordToColorRenderTarget()->image_view,
            static_cast<VkImageLayout>(m_shadow_coord_to_color_pass.GetShadowCoordToColorRenderTarget()->layout));

        m_imgui_pass.RefreshShadowDepth(
            *m_shadow_coord_to_color_pass.GetShadowDepthToColorRenderTarget()->sampler,
            *m_shadow_coord_to_color_pass.GetShadowDepthToColorRenderTarget()->image_view,
            static_cast<VkImageLayout>(m_shadow_coord_to_color_pass.GetShadowDepthToColorRenderTarget()->layout));
    }
} // namespace Meow