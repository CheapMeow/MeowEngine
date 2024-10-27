#include "runtime_window.h"

#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_global_context.h"
#include "function/level/level.h"
#include "function/render/utils/vulkan_initialize_utils.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <imgui.h>

namespace Meow
{
    RuntimeWindow::RuntimeWindow(std::size_t id, GLFWwindow* glfw_window)
        : Window::Window(id, glfw_window)
    {
        CreateSurface();
        CreateSwapChian();
        CreateDescriptorAllocator();
        CreatePerFrameData();
        CreateRenderPass();
        InitImGui();

        OnSize().connect([&](glm::ivec2 new_size) { m_framebuffer_resized = true; });

        OnIconify().connect([&](bool iconified) { m_iconified = iconified; });

        auto& per_frame_data = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer     = per_frame_data.command_buffer;

        std::shared_ptr<Level> level_ptr = g_runtime_global_context.level_system->GetCurrentActiveLevel().lock();

#ifdef MEOW_DEBUG
        if (!level_ptr)
            MEOW_ERROR("shared ptr is invalid!");
#endif

        auto main_camera_id = level_ptr->CreateObject();
        level_ptr->SetMainCameraID(main_camera_id);
        std::shared_ptr<GameObject> camera_go_ptr = level_ptr->GetGameObjectByID(main_camera_id).lock();

#ifdef MEOW_DEBUG
        if (!camera_go_ptr)
            MEOW_ERROR("GameObject is invalid!");
#endif

        camera_go_ptr->SetName("Camera");
        std::shared_ptr<Transform3DComponent> camera_transform_comp_ptr =
            TryAddComponent(camera_go_ptr, "Transform3DComponent", std::make_shared<Transform3DComponent>());
        std::shared_ptr<Camera3DComponent> camera_comp_ptr =
            TryAddComponent(camera_go_ptr, "Camera3DComponent", std::make_shared<Camera3DComponent>());

#ifdef MEOW_DEBUG
        if (!camera_transform_comp_ptr)
            MEOW_ERROR("shared ptr is invalid!");
        if (!camera_comp_ptr)
            MEOW_ERROR("shared ptr is invalid!");
#endif
        camera_transform_comp_ptr->position = glm::vec3(0.0f, 0.0f, -10.0f);

        camera_comp_ptr->camera_mode  = CameraMode::Free;
        camera_comp_ptr->aspect_ratio = (float)m_surface_data.extent.width / m_surface_data.extent.height;

        UUID                        model_go_id  = level_ptr->CreateObject();
        std::shared_ptr<GameObject> model_go_ptr = level_ptr->GetGameObjectByID(model_go_id).lock();

#ifdef MEOW_DEBUG
        if (!model_go_ptr)
            MEOW_ERROR("GameObject is invalid!");
#endif
        model_go_ptr->SetName("Backpack");
        TryAddComponent(model_go_ptr, "Transform3DComponent", std::make_shared<Transform3DComponent>());
        TryAddComponent(model_go_ptr,
                        "ModelComponent",
                        std::make_shared<ModelComponent>("builtin/models/backpack/backpack.obj",
                                                         m_render_pass_ptr->input_vertex_attributes));

        for (int i = 0; i < 200; i++)
        {
            UUID                        model_go_id1  = level_ptr->CreateObject();
            std::shared_ptr<GameObject> model_go_ptr1 = level_ptr->GetGameObjectByID(model_go_id1).lock();

            model_go_ptr1->SetName("Backpack" + std::to_string(i + 1));

#ifdef MEOW_DEBUG
            if (!model_go_ptr1)
                MEOW_ERROR("GameObject is invalid!");
#endif
            std::shared_ptr<Transform3DComponent> model_transform_comp_ptr =
                TryAddComponent(model_go_ptr1, "Transform3DComponent", std::make_shared<Transform3DComponent>());
            TryAddComponent(model_go_ptr1,
                            "ModelComponent",
                            std::make_shared<ModelComponent>("builtin/models/backpack/backpack.obj",
                                                             m_render_pass_ptr->input_vertex_attributes));

            model_transform_comp_ptr->position = glm::vec3(10.0 * glm::linearRand(-1.0f, 1.0f),
                                                           10.0 * glm::linearRand(-1.0f, 1.0f),
                                                           10.0 * glm::linearRand(-1.0f, 1.0f));
        }
    }

    RuntimeWindow::~RuntimeWindow()
    {
        const vk::raii::Device& logical_device = g_runtime_global_context.render_system->GetLogicalDevice();
        logical_device.waitIdle();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_imgui_pass            = nullptr;
        m_imgui_descriptor_pool = nullptr;
        m_per_frame_data.clear();
        m_forward_pass         = nullptr;
        m_deferred_pass        = nullptr;
        m_descriptor_allocator = nullptr;
        m_swapchain_data       = nullptr;
        m_surface_data         = nullptr;
    }

    void RuntimeWindow::Tick(float dt)
    {
        FUNCTION_TIMER();

        if (m_iconified)
            return;

        const vk::raii::Device& logical_device            = g_runtime_global_context.render_system->GetLogicalDevice();
        const vk::raii::Queue&  graphics_queue            = g_runtime_global_context.render_system->GetGraphicsQueue();
        const vk::raii::Queue&  present_queue             = g_runtime_global_context.render_system->GetPresentQueue();
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

        m_render_pass_ptr->Start(cmd_buffer, m_surface_data, m_current_image_index);
        m_render_pass_ptr->Draw(cmd_buffer);
        m_render_pass_ptr->End(cmd_buffer);

        m_imgui_pass.Start(cmd_buffer, m_surface_data, m_current_image_index);
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

    void RuntimeWindow::CreateSurface()
    {
        const vk::raii::Instance& vulkan_instance = g_runtime_global_context.render_system->GetInstance();

        auto         size = GetSize();
        vk::Extent2D extent(size.x, size.y);
        m_surface_data = SurfaceData(vulkan_instance, GetGLFWWindow(), extent);
    }

    void RuntimeWindow::CreateSwapChian()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_global_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_global_context.render_system->GetLogicalDevice();

        m_swapchain_data =
            SwapChainData(physical_device,
                          logical_device,
                          m_surface_data.surface,
                          m_surface_data.extent,
                          vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                          nullptr,
                          g_runtime_global_context.render_system->GetGraphicsQueueFamiliyIndex(),
                          g_runtime_global_context.render_system->GetPresentQueueFamilyIndex());
    }

    void RuntimeWindow::CreateDescriptorAllocator()
    {
        const vk::raii::Device& logical_device = g_runtime_global_context.render_system->GetLogicalDevice();

        // create a descriptor pool
        // TODO: descriptor pool size is determined by all materials, so
        // it depends on analysis of shader?
        // Or you can allocate a very large pool at first?
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
        m_descriptor_allocator                         = DescriptorAllocatorGrowable(logical_device, 1000, pool_sizes);
    }

    void RuntimeWindow::CreatePerFrameData()
    {
        const vk::raii::Device& logical_device = g_runtime_global_context.render_system->GetLogicalDevice();
        const auto graphics_queue_family_index = g_runtime_global_context.render_system->GetGraphicsQueueFamiliyIndex();

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

    void RuntimeWindow::CreateRenderPass()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_global_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_global_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_global_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_global_context.render_system->GetGraphicsQueue();

        m_deferred_pass = DeferredPass(physical_device,
                                       logical_device,
                                       m_surface_data,
                                       onetime_submit_command_pool,
                                       graphics_queue,
                                       m_descriptor_allocator);
        m_deferred_pass.RefreshFrameBuffers(physical_device,
                                            logical_device,
                                            onetime_submit_command_pool,
                                            graphics_queue,
                                            m_surface_data,
                                            m_swapchain_data.image_views,
                                            m_surface_data.extent);

        m_forward_pass = ForwardPass(physical_device,
                                     logical_device,
                                     m_surface_data,
                                     onetime_submit_command_pool,
                                     graphics_queue,
                                     m_descriptor_allocator);
        m_forward_pass.RefreshFrameBuffers(physical_device,
                                           logical_device,
                                           onetime_submit_command_pool,
                                           graphics_queue,
                                           m_surface_data,
                                           m_swapchain_data.image_views,
                                           m_surface_data.extent);

        m_imgui_pass = ImGuiPass(physical_device,
                                 logical_device,
                                 m_surface_data,
                                 onetime_submit_command_pool,
                                 graphics_queue,
                                 m_descriptor_allocator);
        m_imgui_pass.RefreshFrameBuffers(physical_device,
                                         logical_device,
                                         onetime_submit_command_pool,
                                         graphics_queue,
                                         m_surface_data,
                                         m_swapchain_data.image_views,
                                         m_surface_data.extent);

        m_imgui_pass.OnPassChanged().connect([&](int cur_render_pass) {
            // switch render pass
            if (cur_render_pass == 0)
                m_render_pass_ptr = &m_deferred_pass;
            else if (cur_render_pass == 1)
                m_render_pass_ptr = &m_forward_pass;

            g_runtime_global_context.profile_system->ClearProfile();
        });

        m_render_pass_ptr = &m_deferred_pass;
    }

    void RuntimeWindow::InitImGui()
    {
        const vk::raii::Instance&       vulkan_instance = g_runtime_global_context.render_system->GetInstance();
        const vk::raii::PhysicalDevice& physical_device = g_runtime_global_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_global_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_global_context.render_system->GetOneTimeSubmitCommandPool();
        const auto graphics_queue_family_index = g_runtime_global_context.render_system->GetGraphicsQueueFamiliyIndex();
        const vk::raii::Queue& graphics_queue  = g_runtime_global_context.render_system->GetGraphicsQueue();

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

    void RuntimeWindow::RecreateSwapChain()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_global_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_global_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_global_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_global_context.render_system->GetGraphicsQueue();

        logical_device.waitIdle();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_per_frame_data.clear();
        m_swapchain_data = nullptr;
        m_surface_data   = nullptr;

        CreateSurface();
        CreateSwapChian();
        CreatePerFrameData();
        InitImGui();
        m_deferred_pass.RefreshFrameBuffers(physical_device,
                                            logical_device,
                                            onetime_submit_command_pool,
                                            graphics_queue,
                                            m_surface_data,
                                            m_swapchain_data.image_views,
                                            m_surface_data.extent);
        m_forward_pass.RefreshFrameBuffers(physical_device,
                                           logical_device,
                                           onetime_submit_command_pool,
                                           graphics_queue,
                                           m_surface_data,
                                           m_swapchain_data.image_views,
                                           m_surface_data.extent);
        m_imgui_pass.RefreshFrameBuffers(physical_device,
                                         logical_device,
                                         onetime_submit_command_pool,
                                         graphics_queue,
                                         m_surface_data,
                                         m_swapchain_data.image_views,
                                         m_surface_data.extent);

        // update aspect ratio

        std::shared_ptr<Level>      level_ptr = g_runtime_global_context.level_system->GetCurrentActiveLevel().lock();
        std::shared_ptr<GameObject> camera_go_ptr = level_ptr->GetGameObjectByID(level_ptr->GetMainCameraID()).lock();

        if (!camera_go_ptr)
            return;

        std::shared_ptr<Camera3DComponent> camera_comp_ptr =
            camera_go_ptr->TryGetComponent<Camera3DComponent>("Camera3DComponent");

        camera_comp_ptr->aspect_ratio = (float)m_surface_data.extent.width / m_surface_data.extent.height;
    }
} // namespace Meow
