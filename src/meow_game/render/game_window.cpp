#include "game_window.h"

#include "function/components/light/directional_light_component.h"
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
    GameWindow::GameWindow(std::size_t id, GLFWwindow* glfw_window)
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
            directional_light_transform->position = glm::vec3(0.0f, 10.0f, -6.0f);
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

    GameWindow::~GameWindow()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();
        logical_device.waitIdle();

        m_per_frame_data.clear();
        m_shadow_map_pass = nullptr;
        m_forward_pass    = nullptr;
        m_deferred_pass   = nullptr;
        m_swapchain_data  = nullptr;
        m_surface_data    = nullptr;
    }

    void GameWindow::Tick(float dt)
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

        m_render_pass_ptr->Start(cmd_buffer, m_surface_data.extent, m_current_image_index);
        m_render_pass_ptr->Draw(cmd_buffer);
        m_render_pass_ptr->End(cmd_buffer);

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

        Window::Tick(dt);
    }

    void GameWindow::CreateSwapChian()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

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

    void GameWindow::CreatePerFrameData()
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

    void GameWindow::CreateRenderPass()
    {
        m_shadow_map_pass = ShadowMapPass(m_surface_data);
        m_deferred_pass   = GameDeferredPass(m_surface_data);
        m_forward_pass    = GameForwardPass(m_surface_data);
        RefreshFrameBuffers();

        m_render_pass_ptr = &m_forward_pass;
    }

    void GameWindow::RecreateSwapChain()
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

        // update aspect ratio

        std::shared_ptr<Level>      level              = g_runtime_context.level_system->GetCurrentActiveLevel().lock();
        std::shared_ptr<GameObject> current_gameobject = level->GetGameObjectByID(level->GetMainCameraID()).lock();

        if (!current_gameobject)
            return;

        std::shared_ptr<Camera3DComponent> camera_ptr =
            current_gameobject->TryGetComponent<Camera3DComponent>("Camera3DComponent");

        camera_ptr->aspect_ratio = (float)m_surface_data.extent.width / m_surface_data.extent.height;
    }

    void GameWindow::RefreshFrameBuffers()
    {
        std::vector<vk::ImageView> swapchain_image_views;
        swapchain_image_views.resize(m_swapchain_data.image_views.size());
        for (int i = 0; i < m_swapchain_data.image_views.size(); i++)
        {
            swapchain_image_views[i] = *m_swapchain_data.image_views[i];
        }

        m_shadow_map_pass.RefreshFrameBuffers(swapchain_image_views, m_surface_data.extent);
        m_deferred_pass.RefreshFrameBuffers(swapchain_image_views, m_surface_data.extent);
        m_forward_pass.RefreshFrameBuffers(swapchain_image_views, m_surface_data.extent);

        m_forward_pass.BindShadowMap(m_shadow_map_pass.GetShadowMap());
        m_forward_pass.PopulateDirectionalLightData(m_shadow_map_pass.GetShadowMap());
    }
} // namespace Meow