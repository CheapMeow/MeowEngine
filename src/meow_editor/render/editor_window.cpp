#include "editor_window.h"

#include "global/editor_context.h"
#include "meow_runtime/function/components/camera/camera_3d_component.hpp"
#include "meow_runtime/function/components/model/model_component.h"
#include "meow_runtime/function/components/transform/transform_3d_component.hpp"
#include "meow_runtime/function/global/runtime_context.h"
#include "meow_runtime/function/level/level.h"
#include "meow_runtime/function/render/utils/model_utils.h"
#include "meow_runtime/function/render/utils/vulkan_initialization_utils.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <imgui.h>

namespace Meow
{
    EditorWindow::EditorWindow(std::size_t id, GLFWwindow* glfw_window)
        : Window(id, glfw_window)
    {
        CreateSurface();
        CreateSwapChian();
        CreatePerFrameData();
        CreateRenderPass();

#ifdef MEOW_EDITOR
        InitImGui();

        is_offscreen_valid = true;
        m_imgui_pass.RefreshOffscreenRenderTarget(*m_offscreen_render_target->sampler,
                                                  *m_offscreen_render_target->image_view,
                                                  static_cast<VkImageLayout>(m_offscreen_render_target->layout));
#endif

        OnSize().connect([&](glm::ivec2 new_size) { m_framebuffer_resized = true; });

        OnIconify().connect([&](bool iconified) { m_iconified = iconified; });

        auto& per_frame_data = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer     = per_frame_data.command_buffer;

        std::shared_ptr<Level> level_ptr = g_runtime_context.level_system->GetCurrentActiveLevel().lock();

#ifdef MEOW_DEBUG
        if (!level_ptr)
            MEOW_ERROR("shared ptr is invalid!");
#endif

        {
            auto uuid = level_ptr->CreateObject();
            level_ptr->SetMainCameraID(uuid);
            std::shared_ptr<GameObject> gameobject_ptr = level_ptr->GetGameObjectByID(uuid).lock();

#ifdef MEOW_DEBUG
            if (!gameobject_ptr)
                MEOW_ERROR("GameObject is invalid!");
#endif

            gameobject_ptr->SetName("Camera");
            std::shared_ptr<Transform3DComponent> transform_ptr =
                TryAddComponent(gameobject_ptr, "Transform3DComponent", std::make_shared<Transform3DComponent>());
            std::shared_ptr<Camera3DComponent> camera_ptr =
                TryAddComponent(gameobject_ptr, "Camera3DComponent", std::make_shared<Camera3DComponent>());

#ifdef MEOW_DEBUG
            if (!transform_ptr)
                MEOW_ERROR("shared ptr is invalid!");
            if (!camera_ptr)
                MEOW_ERROR("shared ptr is invalid!");
#endif
            transform_ptr->position = glm::vec3(0.0f, 0.0f, -10.0f);

            camera_ptr->camera_mode  = CameraMode::Free;
            camera_ptr->aspect_ratio = (float)m_surface_data.extent.width / m_surface_data.extent.height;
        }

        {
            std::size_t row_number    = 7;
            std::size_t column_number = 7;
            float       spacing       = 2.5;
            auto [sphere_vertices, sphere_indices] =
                GenerateSphereVerticesAndIndices(64, 64, 1.0f, m_render_pass_ptr->input_vertex_attributes);
            for (std::size_t row = 0; row < row_number; ++row)
            {
                for (std::size_t col = 0; col < column_number; ++col)
                {
                    UUID                        uuid           = level_ptr->CreateObject();
                    std::shared_ptr<GameObject> gameobject_ptr = level_ptr->GetGameObjectByID(uuid).lock();

                    opaque_objects.push_back(gameobject_ptr);

#ifdef MEOW_DEBUG
                    if (!gameobject_ptr)
                        MEOW_ERROR("GameObject is invalid!");
#endif
                    gameobject_ptr->SetName("Sphere " + std::to_string(row * column_number + col));
                    auto transform_ptr = TryAddComponent(
                        gameobject_ptr, "Transform3DComponent", std::make_shared<Transform3DComponent>());
                    transform_ptr->position = glm::vec3((float)col * spacing - (float)column_number / 2.0f * spacing,
                                                        (float)row * spacing - (float)row_number / 2.0f * spacing,
                                                        0.0f);

                    auto model_comp_ptr =
                        TryAddComponent(gameobject_ptr, "ModelComponent", std::make_shared<ModelComponent>());
                    auto model_shared_ptr = std::make_shared<Model>(
                        sphere_vertices, sphere_indices, m_render_pass_ptr->input_vertex_attributes);

                    // TODO: hard code render pass cast
                    model_comp_ptr->material_id = m_forward_pass.GetForwardMatID();

                    if (model_shared_ptr)
                    {
                        g_runtime_context.resource_system->Register(model_shared_ptr);
                        model_comp_ptr->model_ptr = model_shared_ptr;
                    }
                }
            }
        }

        {
            std::size_t row_number    = 7;
            std::size_t column_number = 7;
            float       spacing       = 2.5;
            auto [sphere_vertices, sphere_indices] =
                GenerateSphereVerticesAndIndices(64, 64, 1.0f, m_render_pass_ptr->input_vertex_attributes);
            for (std::size_t row = 0; row < row_number; ++row)
            {
                for (std::size_t col = 0; col < column_number; ++col)
                {
                    UUID                        uuid           = level_ptr->CreateObject();
                    std::shared_ptr<GameObject> gameobject_ptr = level_ptr->GetGameObjectByID(uuid).lock();

                    translucent_objects.push_back(gameobject_ptr);

#ifdef MEOW_DEBUG
                    if (!gameobject_ptr)
                        MEOW_ERROR("GameObject is invalid!");
#endif
                    gameobject_ptr->SetName("Sphere Translucent" + std::to_string(row * column_number + col));
                    auto transform_ptr = TryAddComponent(
                        gameobject_ptr, "Transform3DComponent", std::make_shared<Transform3DComponent>());
                    transform_ptr->position = glm::vec3((float)col * spacing - (float)column_number / 2.0f * spacing,
                                                        (float)row * spacing - (float)row_number / 2.0f * spacing,
                                                        -5.0f);

                    auto model_comp_ptr =
                        TryAddComponent(gameobject_ptr, "ModelComponent", std::make_shared<ModelComponent>());
                    auto model_shared_ptr = std::make_shared<Model>(
                        sphere_vertices, sphere_indices, m_render_pass_ptr->input_vertex_attributes);

                    // TODO: hard code render pass cast
                    model_comp_ptr->material_id = m_forward_pass.GetTranslucentMatID();

                    if (model_shared_ptr)
                    {
                        g_runtime_context.resource_system->Register(model_shared_ptr);
                        model_comp_ptr->model_ptr = model_shared_ptr;
                    }
                }
            }
        }
    }

    EditorWindow::~EditorWindow()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();
        logical_device.waitIdle();

#ifdef MEOW_EDITOR
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_imgui_pass            = nullptr;
        m_imgui_descriptor_pool = nullptr;
#endif

        m_per_frame_data.clear();
        m_forward_pass   = nullptr;
        m_deferred_pass  = nullptr;
        m_swapchain_data = nullptr;
        m_surface_data   = nullptr;
    }

    void EditorWindow::Tick(float dt)
    {
        FUNCTION_TIMER();

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

        m_render_pass_ptr->UpdateUniformBuffer();

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
        cmd_buffer.setViewport(0,
                               vk::Viewport(0.0f,
                                            static_cast<float>(m_surface_data.extent.height),
                                            static_cast<float>(m_surface_data.extent.width),
                                            -static_cast<float>(m_surface_data.extent.height),
                                            0.0f,
                                            1.0f));
        cmd_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_surface_data.extent));

#ifdef MEOW_EDITOR
        // TODO: temp

        vk::Extent2D temp_extent = {m_surface_data.extent.width / 2, m_surface_data.extent.height / 2};

        m_render_pass_ptr->Start(cmd_buffer, temp_extent, 0);
#else
        m_render_pass_ptr->Start(cmd_buffer, m_surface_data.extent, m_current_image_index);
#endif
        m_render_pass_ptr->Draw(cmd_buffer);
        m_render_pass_ptr->End(cmd_buffer);

#ifdef MEOW_EDITOR
        m_imgui_pass.Start(cmd_buffer, m_surface_data.extent, m_current_image_index);
        m_imgui_pass.Draw(cmd_buffer);
        m_imgui_pass.End(cmd_buffer);
#endif

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

#ifdef MEOW_EDITOR
        m_imgui_pass.AfterPresent();
#endif
        Window::Tick(dt);
    }

    void EditorWindow::CreateSurface()
    {
        const vk::raii::Instance& vulkan_instance = g_runtime_context.render_system->GetInstance();

        auto         size = GetSize();
        vk::Extent2D extent(size.x, size.y);
        m_surface_data = SurfaceData(vulkan_instance, GetGLFWWindow(), extent);
    }

    void EditorWindow::CreateSwapChian()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*m_surface_data.surface)).format;

        m_swapchain_data =
            SwapChainData(physical_device,
                          logical_device,
                          m_surface_data.surface,
                          m_surface_data.extent,
                          vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                          nullptr,
                          g_runtime_context.render_system->GetGraphicsQueueFamiliyIndex(),
                          g_runtime_context.render_system->GetPresentQueueFamilyIndex());

        // TODO: temp
#ifdef MEOW_EDITOR
        vk::Extent2D temp_extent = {m_surface_data.extent.width / 2, m_surface_data.extent.height / 2};

        m_offscreen_render_target = ImageData::CreateRenderTarget(color_format,
                                                                  temp_extent,
                                                                  vk::ImageUsageFlagBits::eColorAttachment |
                                                                      vk::ImageUsageFlagBits::eInputAttachment,
                                                                  vk::ImageAspectFlagBits::eColor,
                                                                  {},
                                                                  false);
#endif
    }

    void EditorWindow::CreatePerFrameData()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();
        const auto graphics_queue_family_index = g_runtime_context.render_system->GetGraphicsQueueFamiliyIndex();

        m_per_frame_data.resize(k_max_frames_in_flight);
        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                               graphics_queue_family_index);
            m_per_frame_data[i].command_pool = vk::raii::CommandPool(logical_device, command_pool_create_info);

            vk::CommandBufferAllocateInfo command_buffer_allocate_info(
                *m_per_frame_data[i].command_pool, vk::CommandBufferLevel::ePrimary, 1);
            vk::raii::CommandBuffers command_buffers(logical_device, command_buffer_allocate_info);
            m_per_frame_data[i].command_buffer = std::move(command_buffers[0]);

            m_per_frame_data[i].image_acquired_semaphore =
                vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo());
            m_per_frame_data[i].render_finished_semaphore =
                vk::raii::Semaphore(logical_device, vk::SemaphoreCreateInfo());
            m_per_frame_data[i].in_flight_fence = vk::raii::Fence(logical_device, vk::FenceCreateInfo());
        }
    }

    void EditorWindow::CreateRenderPass()
    {
        m_deferred_pass = EditorDeferredPass(m_surface_data);
        m_forward_pass  = EditorForwardPass(m_surface_data);
#ifdef MEOW_EDITOR
        m_imgui_pass = ImGuiPass(m_surface_data);
#endif

        RefreshRenderPass();

#ifdef MEOW_EDITOR
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
                    auto model_comp_ptr = opaque_object_ptr->TryGetComponent<ModelComponent>("ModelComponent");

                    if (cur_render_pass == 1)
                        model_comp_ptr->material_id = m_forward_pass.GetForwardMatID();
                    else if (cur_render_pass == 0)
                        model_comp_ptr->material_id = m_deferred_pass.GetObj2AttachmentMatID();
                }
            }
        });
#endif

        m_render_pass_ptr = &m_forward_pass;
    }

#ifdef MEOW_EDITOR
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
#endif

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
        RefreshRenderPass();

        // update aspect ratio

        std::shared_ptr<Level>      level_ptr      = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        std::shared_ptr<GameObject> gameobject_ptr = level_ptr->GetGameObjectByID(level_ptr->GetMainCameraID()).lock();

        if (!gameobject_ptr)
            return;

        std::shared_ptr<Camera3DComponent> camera_ptr =
            gameobject_ptr->TryGetComponent<Camera3DComponent>("Camera3DComponent");

        camera_ptr->aspect_ratio = (float)m_surface_data.extent.width / m_surface_data.extent.height;
    }

    void EditorWindow::RefreshRenderPass()
    {
        std::vector<vk::ImageView> swapchain_image_views;
        swapchain_image_views.resize(m_swapchain_data.image_views.size());
        for (int i = 0; i < m_swapchain_data.image_views.size(); i++)
        {
            swapchain_image_views[i] = *m_swapchain_data.image_views[i];
        }

        // TODO: temp
#ifdef MEOW_EDITOR
        vk::Extent2D temp_extent = {m_surface_data.extent.width / 2, m_surface_data.extent.height / 2};

        m_deferred_pass.RefreshFrameBuffers({*m_offscreen_render_target->image_view}, temp_extent);
        m_forward_pass.RefreshFrameBuffers({*m_offscreen_render_target->image_view}, temp_extent);
        m_imgui_pass.RefreshFrameBuffers(swapchain_image_views, m_surface_data.extent);

        if (is_offscreen_valid)
            m_imgui_pass.RefreshOffscreenRenderTarget(*m_offscreen_render_target->sampler,
                                                      *m_offscreen_render_target->image_view,
                                                      static_cast<VkImageLayout>(m_offscreen_render_target->layout));
#else
        m_deferred_pass.RefreshFrameBuffers(swapchain_image_views, m_surface_data.extent);
        m_forward_pass.RefreshFrameBuffers(swapchain_image_views, m_surface_data.extent);
#endif
    }
} // namespace Meow