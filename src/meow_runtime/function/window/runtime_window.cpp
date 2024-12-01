#include "runtime_window.h"

#include "function/components/camera/camera_3d_component.hpp"
#include "function/components/model/model_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_context.h"
#include "function/level/level.h"
#include "function/render/utils/model_utils.h"
#include "function/render/utils/vulkan_initialize_utils.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <imgui.h>

namespace Meow
{
    RuntimeWindow::RuntimeWindow(std::size_t id, GLFWwindow* glfw_window)
        : Window(id, glfw_window)
    {
        CreateSurface();
        CreateSwapChian();
        CreatePerFrameData();
        CreateRenderPass();

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
                GenerateSphereVerticesAndIndices(64, 64, 1.0f, g_runtime_context.render_system->GetVertexAttributes());
            for (std::size_t row = 0; row < row_number; ++row)
            {
                for (std::size_t col = 0; col < column_number; ++col)
                {
                    UUID                        uuid           = level_ptr->CreateObject();
                    std::shared_ptr<GameObject> gameobject_ptr = level_ptr->GetGameObjectByID(uuid).lock();

#ifdef MEOW_DEBUG
                    if (!gameobject_ptr)
                        MEOW_ERROR("GameObject is invalid!");
#endif
                    gameobject_ptr->SetName("Sphere");
                    auto transform_ptr = TryAddComponent(
                        gameobject_ptr, "Transform3DComponent", std::make_shared<Transform3DComponent>());
                    transform_ptr->position = glm::vec3((float)col * spacing - (float)column_number / 2.0f * spacing,
                                                        (float)row * spacing - (float)row_number / 2.0f * spacing,
                                                        0.0f);

                    auto model_comp_ptr =
                        TryAddComponent(gameobject_ptr, "ModelComponent", std::make_shared<ModelComponent>());
                    auto model_shared_ptr = std::make_shared<Model>(
                        sphere_vertices, sphere_indices, g_runtime_context.render_system->GetVertexAttributes());
                    if (model_shared_ptr)
                    {
                        g_runtime_context.resource_system->Register(model_shared_ptr);
                        model_comp_ptr->model_ptr = model_shared_ptr;
                    }
                }
            }
        }
    }

    RuntimeWindow::~RuntimeWindow()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();
        logical_device.waitIdle();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_per_frame_data.clear();
        m_swapchain_data = nullptr;
        m_surface_data   = nullptr;
    }

    void RuntimeWindow::Tick(float dt)
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

        if (m_cur_render_path == 0)
            m_forward_path.UpdateUniformBuffer();
        else if (m_cur_render_path == 1)
            m_deferred_path.UpdateUniformBuffer();

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

        if (m_cur_render_path == 0)
            m_forward_path.Draw(cmd_buffer, m_surface_data.extent, m_swapchain_data.images[m_current_image_index]);
        else if (m_cur_render_path == 1)
            m_deferred_path.Draw(cmd_buffer, m_surface_data.extent, m_swapchain_data.images[m_current_image_index]);

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

        if (m_cur_render_path == 0)
            m_forward_path.AfterPresent();
        else if (m_cur_render_path == 1)
            m_deferred_path.AfterPresent();

        Window::Tick(dt);
    }

    void RuntimeWindow::CreateSurface()
    {
        const vk::raii::Instance& vulkan_instance = g_runtime_context.render_system->GetInstance();

        auto         size = GetSize();
        vk::Extent2D extent(size.x, size.y);
        m_surface_data = SurfaceData(vulkan_instance, GetGLFWWindow(), extent);
    }

    void RuntimeWindow::CreateSwapChian()
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
    }

    void RuntimeWindow::CreatePerFrameData()
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

    void RuntimeWindow::CreateRenderPass()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();

        m_forward_path  = std::move(ForwardPath());
        m_deferred_path = std::move(DeferredPath());

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*m_surface_data.surface)).format;
        assert(color_format != vk::Format::eUndefined);

        RefreshAttachments();

#ifdef MEOW_EDITOR
        m_forward_path.GetImGuiPass().OnPassChanged().connect([&](int cur_render_path) {
            // switch render path
            m_cur_render_path = cur_render_path;

            g_runtime_context.profile_system->ClearProfile();
        });
        m_deferred_path.GetImGuiPass().OnPassChanged().connect([&](int cur_render_path) {
            // switch render path
            m_cur_render_path = cur_render_path;

            g_runtime_context.profile_system->ClearProfile();
        });
#endif
    }

    void RuntimeWindow::RecreateSwapChain()
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
        RefreshAttachments();

        // update aspect ratio

        std::shared_ptr<Level>      level_ptr      = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        std::shared_ptr<GameObject> gameobject_ptr = level_ptr->GetGameObjectByID(level_ptr->GetMainCameraID()).lock();

        if (!gameobject_ptr)
            return;

        std::shared_ptr<Camera3DComponent> camera_ptr =
            gameobject_ptr->TryGetComponent<Camera3DComponent>("Camera3DComponent");

        camera_ptr->aspect_ratio = (float)m_surface_data.extent.width / m_surface_data.extent.height;
    }

    void RuntimeWindow::RefreshAttachments()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();

        vk::Format color_format =
            PickSurfaceFormat((physical_device).getSurfaceFormatsKHR(*m_surface_data.surface)).format;

        m_forward_path.RefreshAttachments(color_format, m_surface_data.extent);
        m_deferred_path.RefreshAttachments(color_format, m_surface_data.extent);
    }
} // namespace Meow