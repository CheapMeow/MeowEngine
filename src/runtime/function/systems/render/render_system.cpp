#include "render_system.h"

#include "core/log/log.h"
#include "function/components/3d/camera/camera_3d_component.h"
#include "function/components/3d/model/model_component.h"
#include "function/components/3d/transform/transform_3d_component.h"
#include "function/components/shared/pawn_component.h"
#include "function/global/runtime_global_context.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <volk.h>

namespace Meow
{
    RenderSystem::RenderSystem()
    {
        CreateVulkanContent();
        CreateVulkanInstance();
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        CreateDebugUtilsMessengerEXT();
#endif
        CreatePhysicalDevice();
        CreateSurface();
        CreateLogicalDevice();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSwapChian();
        CreateDepthBuffer();
        CreateUniformBuffer();
        CreateDescriptorSetLayout();
        CreatePipelineLayout();
        CreateDescriptorPool();
        CreateDescriptorSet();
        CreateRenderPass();
        CreateFramebuffers();
        CreatePipeline();
        CreateSyncObjects();
        InitImGui();

        // TODO: creating entity should be obstract
        const auto camera_entity = g_runtime_global_context.registry.create();
        auto&      camera_transform_component =
            g_runtime_global_context.registry.emplace<Transform3DComponent>(camera_entity);
        auto& camera_component          = g_runtime_global_context.registry.emplace<Camera3DComponent>(camera_entity);
        camera_component.is_main_camera = true;

        const auto model_entity = g_runtime_global_context.registry.create();
        g_runtime_global_context.registry.emplace<Transform3DComponent>(model_entity);
        // model_transform_component.global_transform =
        //     glm::rotate(model_transform_component.global_transform, glm::radians(180.0f), glm::vec3(0.0f, 1.0f,
        //     0.0f));
        auto& model_component = g_runtime_global_context.registry.emplace<ModelComponent>(
            model_entity,
            "builtin/models/monkey_head.obj",
            *g_runtime_global_context.render_context.physical_device,
            *g_runtime_global_context.render_context.logical_device,
            std::vector<VertexAttribute> {VertexAttribute::VA_Position, VertexAttribute::VA_Normal});

        BoundingBox model_bounding          = model_component.model.GetBounding();
        glm::vec3   bound_size              = model_bounding.max - model_bounding.min;
        glm::vec3   bound_center            = model_bounding.min + bound_size * 0.5f;
        camera_transform_component.position = bound_center + glm::vec3(0.0f, 0.0f, -50.0f);
        // glm::lookAt
    }

    RenderSystem::~RenderSystem()
    {
        g_runtime_global_context.render_context.logical_device->waitIdle();

        // TODO: deleting entity should be obstract
        auto model_view = g_runtime_global_context.registry.view<Transform3DComponent, ModelComponent>();
        g_runtime_global_context.registry.destroy(model_view.begin(), model_view.end());

        auto camera_view = g_runtime_global_context.registry.view<Transform3DComponent, Camera3DComponent>();
        g_runtime_global_context.registry.destroy(camera_view.begin(), camera_view.end());

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        g_runtime_global_context.render_context.image_acquired_semaphores.clear();
        g_runtime_global_context.render_context.render_finished_semaphores.clear();
        g_runtime_global_context.render_context.in_flight_fences.clear();
        g_runtime_global_context.render_context.graphics_pipeline = nullptr;
        g_runtime_global_context.render_context.framebuffers.clear();
        g_runtime_global_context.render_context.render_pass           = nullptr;
        g_runtime_global_context.render_context.descriptor_set        = nullptr;
        g_runtime_global_context.render_context.descriptor_pool       = nullptr;
        g_runtime_global_context.render_context.imgui_descriptor_pool = nullptr;
        g_runtime_global_context.render_context.pipeline_layout       = nullptr;
        g_runtime_global_context.render_context.descriptor_set_layout = nullptr;
        g_runtime_global_context.render_context.uniform_buffer_data   = nullptr;
        g_runtime_global_context.render_context.depth_buffer_data     = nullptr;
        g_runtime_global_context.render_context.swapchain_data        = nullptr;
        g_runtime_global_context.render_context.command_buffers.clear();
        g_runtime_global_context.render_context.command_pool    = nullptr;
        g_runtime_global_context.render_context.present_queue   = nullptr;
        g_runtime_global_context.render_context.graphics_queue  = nullptr;
        g_runtime_global_context.render_context.logical_device  = nullptr;
        g_runtime_global_context.render_context.surface_data    = nullptr;
        g_runtime_global_context.render_context.physical_device = nullptr;
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        g_runtime_global_context.render_context.debug_utils_messenger = nullptr;
#endif
        g_runtime_global_context.render_context.vulkan_instance = nullptr;
        g_runtime_global_context.render_context.vulkan_context  = nullptr;
    }

    void RenderSystem::UpdateUniformBuffer(UBOData ubo_data)
    {
        vk::Meow::CopyToDevice(g_runtime_global_context.render_context.uniform_buffer_data->device_memory, ubo_data);
        vk::Meow::UpdateDescriptorSets(*g_runtime_global_context.render_context.logical_device,
                                       *g_runtime_global_context.render_context.descriptor_set,
                                       {{vk::DescriptorType::eUniformBuffer,
                                         g_runtime_global_context.render_context.uniform_buffer_data->buffer,
                                         VK_WHOLE_SIZE,
                                         nullptr}},
                                       {});
    }

    /**
     * @brief Begin command buffer and render pass. Set viewport and scissor.
     */
    bool RenderSystem::StartRenderpass(uint32_t& image_index)
    {
        vk::Result result;
        std::tie(result, image_index) =
            g_runtime_global_context.render_context.swapchain_data->swap_chain.acquireNextImage(
                vk::Meow::k_fence_timeout,
                **g_runtime_global_context.render_context
                      .image_acquired_semaphores[g_runtime_global_context.render_context.current_frame_index]);
        assert(result == vk::Result::eSuccess);
        assert(image_index < g_runtime_global_context.render_context.swapchain_data->images.size());

        g_runtime_global_context.render_context
            .command_buffers[g_runtime_global_context.render_context.current_frame_index]
            .reset();
        g_runtime_global_context.render_context
            .command_buffers[g_runtime_global_context.render_context.current_frame_index]
            .begin({});

        g_runtime_global_context.render_context
            .command_buffers[g_runtime_global_context.render_context.current_frame_index]
            .setViewport(
                0,
                vk::Viewport(0.0f,
                             static_cast<float>(g_runtime_global_context.render_context.surface_data->extent.height),
                             static_cast<float>(g_runtime_global_context.render_context.surface_data->extent.width),
                             -static_cast<float>(g_runtime_global_context.render_context.surface_data->extent.height),
                             0.0f,
                             1.0f));
        g_runtime_global_context.render_context
            .command_buffers[g_runtime_global_context.render_context.current_frame_index]
            .setScissor(0,
                        vk::Rect2D(vk::Offset2D(0, 0), g_runtime_global_context.render_context.surface_data->extent));

        std::array<vk::ClearValue, 2> clear_values;
        clear_values[0].color        = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 0.2f);
        clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderPassBeginInfo render_pass_begin_info(
            **g_runtime_global_context.render_context.render_pass,
            *g_runtime_global_context.render_context.framebuffers[image_index],
            vk::Rect2D(vk::Offset2D(0, 0), g_runtime_global_context.render_context.surface_data->extent),
            clear_values);
        g_runtime_global_context.render_context
            .command_buffers[g_runtime_global_context.render_context.current_frame_index]
            .beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        g_runtime_global_context.render_context
            .command_buffers[g_runtime_global_context.render_context.current_frame_index]
            .bindPipeline(vk::PipelineBindPoint::eGraphics,
                          **g_runtime_global_context.render_context.graphics_pipeline);
        g_runtime_global_context.render_context
            .command_buffers[g_runtime_global_context.render_context.current_frame_index]
            .bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                **g_runtime_global_context.render_context.pipeline_layout,
                                0,
                                {**g_runtime_global_context.render_context.descriptor_set},
                                nullptr);

        return true;
    }

    /**
     * @brief End render pass and command buffer. Submit graphics queue. Present.
     */
    void RenderSystem::EndRenderpass(uint32_t& image_index)
    {
        g_runtime_global_context.render_context
            .command_buffers[g_runtime_global_context.render_context.current_frame_index]
            .endRenderPass();
        g_runtime_global_context.render_context
            .command_buffers[g_runtime_global_context.render_context.current_frame_index]
            .end();

        vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo         submit_info(
            **g_runtime_global_context.render_context
                  .image_acquired_semaphores[g_runtime_global_context.render_context.current_frame_index],
            wait_destination_stage_mask,
            *g_runtime_global_context.render_context
                 .command_buffers[g_runtime_global_context.render_context.current_frame_index],
            **g_runtime_global_context.render_context
                  .render_finished_semaphores[g_runtime_global_context.render_context.current_frame_index]);
        g_runtime_global_context.render_context.graphics_queue->submit(
            submit_info,
            **g_runtime_global_context.render_context
                  .in_flight_fences[g_runtime_global_context.render_context.current_frame_index]);

        while (vk::Result::eTimeout ==
               g_runtime_global_context.render_context.logical_device->waitForFences(
                   {**g_runtime_global_context.render_context
                          .in_flight_fences[g_runtime_global_context.render_context.current_frame_index]},
                   VK_TRUE,
                   vk::Meow::k_fence_timeout))
            ;
        g_runtime_global_context.render_context.logical_device->resetFences(
            {**g_runtime_global_context.render_context
                   .in_flight_fences[g_runtime_global_context.render_context.current_frame_index]});

        vk::PresentInfoKHR present_info(
            **g_runtime_global_context.render_context
                  .render_finished_semaphores[g_runtime_global_context.render_context.current_frame_index],
            *g_runtime_global_context.render_context.swapchain_data->swap_chain,
            image_index);
        vk::Result result = g_runtime_global_context.render_context.present_queue->presentKHR(present_info);
        switch (result)
        {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n";
                break;
            default:
                assert(false); // an unexpected result is returned !
        }

        g_runtime_global_context.render_context.current_frame_index =
            (g_runtime_global_context.render_context.current_frame_index + 1) % vk::Meow::k_max_frames_in_flight;
    }

    void RenderSystem::Update(float frame_time)
    {
        // ------------------- camera -------------------

        UBOData ubo_data;

        for (auto [entity, transfrom_component, model_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, ModelComponent>().each())
        {
            // TODO: temp
            ubo_data.model = transfrom_component.GetTransform();
        }

        for (auto [entity, transfrom_component, camera_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, const Camera3DComponent>().each())
        {
            if (camera_component.is_main_camera)
            {
                glm::ivec2 window_size = g_runtime_global_context.render_context.window->GetSize();

                glm::mat4 view = glm::mat4(1.0f);
                view           = glm::mat4_cast(glm::conjugate(transfrom_component.rotation)) * view;
                view           = glm::translate(view, -transfrom_component.position);

                ubo_data.view       = view;
                ubo_data.projection = glm::perspectiveLH_ZO(camera_component.field_of_view,
                                                            (float)window_size[0] / (float)window_size[1],
                                                            camera_component.near_plane,
                                                            camera_component.far_plane);
                break;
            }
        }

        // ------------------- ImGui -------------------

        // TODO: temp
        static bool show_demo_window = true;

        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code
        // to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // ------------------- render -------------------

        UpdateUniformBuffer(ubo_data);

        uint32_t image_index;
        StartRenderpass(image_index);

        auto& cmd_buffer = g_runtime_global_context.render_context
                               .command_buffers[g_runtime_global_context.render_context.current_frame_index];

        for (auto [entity, transfrom_component, model_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, ModelComponent>().each())
        {
            // TODO: How to solve the different uniform buffer problem?
            // ubo_data.model = transfrom_component.m_global_transform;

            model_component.model.Draw(cmd_buffer);
        }

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd_buffer);

        // Specially for docking branch
        // Update and Render additional Platform Windows
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        EndRenderpass(image_index);
    }

    void RenderSystem::CreateVulkanContent()
    {
        g_runtime_global_context.render_context.vulkan_context = std::make_shared<vk::raii::Context>();

#ifdef MEOW_DEBUG
        vk::Meow::LogVulkanAPIVersion(
            g_runtime_global_context.render_context.vulkan_context->enumerateInstanceVersion());
#endif
    }

    void RenderSystem::CreateVulkanInstance()
    {
        // prepare for create vk::InstanceCreateInfo

        std::vector<vk::ExtensionProperties> available_instance_extensions =
            g_runtime_global_context.render_context.vulkan_context->enumerateInstanceExtensionProperties();

        std::vector<const char*> required_instance_extensions =
            vk::Meow::GetRequiredInstanceExtensions({VK_KHR_SURFACE_EXTENSION_NAME});

        if (!vk::Meow::ValidateExtensions(required_instance_extensions, available_instance_extensions))
        {
            throw std::runtime_error("Required instance extensions are missing.");
        }

        std::vector<vk::LayerProperties> supported_validation_layers =
            g_runtime_global_context.render_context.vulkan_context->enumerateInstanceLayerProperties();

        std::vector<const char*> required_validation_layers {};

#ifdef VKB_VALIDATION_LAYERS
        // Determine the optimal validation layers to enable that are necessary for useful debugging
        std::vector<const char*> optimal_validation_layers =
            vk::Meow::GetOptimalValidationLayers(supported_validation_layers);
        required_validation_layers.insert(
            required_validation_layers.end(), optimal_validation_layers.begin(), optimal_validation_layers.end());
#endif

        if (vk::Meow::ValidateLayers(required_validation_layers, supported_validation_layers))
        {
            RUNTIME_INFO("Enabled Validation Layers:");
            for (const auto& layer : required_validation_layers)
            {
                RUNTIME_INFO("	\t{}", layer);
            }
        }
        else
        {
            throw std::runtime_error("Required validation layers are missing.");
        }

        uint32_t api_version = g_runtime_global_context.render_context.vulkan_context->enumerateInstanceVersion();

        vk::ApplicationInfo app("Meow Engine Vulkan Renderer", {}, "Meow Engine", {}, api_version);

        vk::InstanceCreateInfo instance_info({}, &app, required_validation_layers, required_instance_extensions);
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        // VkDebugUtilsMessengerEXT only covers stuff from its creation to its destruction.
        // vkCreateInstance and vkDestroyInstance are covered by the special pNext variant
        // because at that point the VkDebugUtilsMessengerEXT object cannot even exist yet\anymore
        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info = vk::Meow::MakeDebugUtilsMessengerCreateInfoEXT();
        instance_info.pNext                                          = &debug_utils_create_info;
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
        instance_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

        g_runtime_global_context.render_context.vulkan_instance = std::make_shared<vk::raii::Instance>(
            *g_runtime_global_context.render_context.vulkan_context, instance_info);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        // we need some additional initializing for this platform!
        if (volkInitialize())
        {
            throw std::runtime_error("Failed to initialize volk.");
        }
        volkLoadInstance(*g_runtime_global_context.render_context.vulkan_instance);
#endif
    }

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
    void RenderSystem::CreateDebugUtilsMessengerEXT()
    {
        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info = vk::Meow::MakeDebugUtilsMessengerCreateInfoEXT();
        g_runtime_global_context.render_context.debug_utils_messenger =
            std::make_shared<vk::raii::DebugUtilsMessengerEXT>(*g_runtime_global_context.render_context.vulkan_instance,
                                                               debug_utils_create_info);
    }
#endif

    void RenderSystem::CreatePhysicalDevice()
    {
        vk::raii::PhysicalDevices physical_devices(*g_runtime_global_context.render_context.vulkan_instance);

        // Maps to hold devices and sort by rank.
        std::multimap<uint32_t, vk::raii::PhysicalDevice> ranked_devices;
        auto                                              where = ranked_devices.end();

        // Iterates through all devices and rate their suitability.
        for (const auto& physical_device : physical_devices)
            where = ranked_devices.insert(
                where,
                {vk::Meow::ScorePhysicalDevice(physical_device, vk::Meow::k_required_device_extensions),
                 physical_device});

        // Checks to make sure the best candidate scored higher than 0  rbegin points to last element of ranked
        // devices(highest rated), first is its rating.
        if (ranked_devices.rbegin()->first < 0)
        {
            throw std::runtime_error("Best gpu get negitive score.");
        }

        g_runtime_global_context.render_context.physical_device =
            std::make_shared<vk::raii::PhysicalDevice>(ranked_devices.rbegin()->second);
    }

    void RenderSystem::CreateSurface()
    {
        auto         size = g_runtime_global_context.render_context.window->GetSize();
        vk::Extent2D extent(size.x, size.y);

        g_runtime_global_context.render_context.surface_data =
            std::make_shared<vk::Meow::SurfaceData>(*g_runtime_global_context.render_context.vulkan_instance,
                                                    g_runtime_global_context.render_context.window->GetGLFWWindow(),
                                                    extent);
    }

    void RenderSystem::CreateLogicalDevice()
    {
        std::vector<vk::ExtensionProperties> device_extensions =
            g_runtime_global_context.render_context.physical_device->enumerateDeviceExtensionProperties();

        if (!vk::Meow::ValidateExtensions(vk::Meow::k_required_device_extensions, device_extensions))
        {
            throw std::runtime_error("Required device extensions are missing, will try without.");
        }

        auto indexs = vk::Meow::FindGraphicsAndPresentQueueFamilyIndex(
            *g_runtime_global_context.render_context.physical_device,
            g_runtime_global_context.render_context.surface_data->surface);
        g_runtime_global_context.render_context.graphics_queue_family_index = indexs.first;
        g_runtime_global_context.render_context.present_queue_family_index  = indexs.second;

        // Create a device with one queue
        float                     queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_info(
            {}, g_runtime_global_context.render_context.graphics_queue_family_index, 1, &queue_priority);
        vk::DeviceCreateInfo device_info({}, queue_info, {}, vk::Meow::k_required_device_extensions);

        g_runtime_global_context.render_context.logical_device =
            std::make_shared<vk::raii::Device>(*g_runtime_global_context.render_context.physical_device, device_info);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        volkLoadDevice(**g_runtime_global_context.render_context.logical_device);
#endif

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Render System Logical Device";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {
            vk::ObjectType::eDevice,
            vk::Meow::GetVulkanHandle(**g_runtime_global_context.render_context.logical_device),
            object_name.c_str(),
            nullptr};
        g_runtime_global_context.render_context.logical_device->setDebugUtilsObjectNameEXT(name_info);
#endif

        g_runtime_global_context.render_context.graphics_queue =
            std::make_shared<vk::raii::Queue>(*g_runtime_global_context.render_context.logical_device,
                                              g_runtime_global_context.render_context.graphics_queue_family_index,
                                              0);
        g_runtime_global_context.render_context.present_queue =
            std::make_shared<vk::raii::Queue>(*g_runtime_global_context.render_context.logical_device,
                                              g_runtime_global_context.render_context.present_queue_family_index,
                                              0);
    }

    void RenderSystem::CreateCommandPool()
    {
        vk::CommandPoolCreateInfo command_pool_create_info(
            vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
            g_runtime_global_context.render_context.graphics_queue_family_index);

        g_runtime_global_context.render_context.command_pool = std::make_shared<vk::raii::CommandPool>(
            *g_runtime_global_context.render_context.logical_device, command_pool_create_info);
    }

    void RenderSystem::CreateCommandBuffers()
    {
        vk::CommandBufferAllocateInfo command_buffer_allocate_info(
            **g_runtime_global_context.render_context.command_pool,
            vk::CommandBufferLevel::ePrimary,
            vk::Meow::k_max_frames_in_flight);

        g_runtime_global_context.render_context.command_buffers = vk::raii::CommandBuffers(
            *g_runtime_global_context.render_context.logical_device, command_buffer_allocate_info);
    }

    void RenderSystem::CreateSwapChian()
    {
        g_runtime_global_context.render_context.swapchain_data = std::make_shared<vk::Meow::SwapChainData>(
            *g_runtime_global_context.render_context.physical_device,
            *g_runtime_global_context.render_context.logical_device,
            g_runtime_global_context.render_context.surface_data->surface,
            g_runtime_global_context.render_context.surface_data->extent,
            vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
            nullptr,
            g_runtime_global_context.render_context.graphics_queue_family_index,
            g_runtime_global_context.render_context.present_queue_family_index);
    }

    void RenderSystem::CreateDepthBuffer()
    {
        g_runtime_global_context.render_context.depth_buffer_data =
            std::make_shared<vk::Meow::DepthBufferData>(*g_runtime_global_context.render_context.physical_device,
                                                        *g_runtime_global_context.render_context.logical_device,
                                                        vk::Format::eD16Unorm,
                                                        g_runtime_global_context.render_context.surface_data->extent);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Render System Depth Image";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {
            vk::ObjectType::eImage,
            vk::Meow::GetVulkanHandle(*g_runtime_global_context.render_context.depth_buffer_data->image),
            object_name.c_str(),
            nullptr};
        g_runtime_global_context.render_context.logical_device->setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void RenderSystem::CreateUniformBuffer()
    {
        // TODO: UBOData is temp
        g_runtime_global_context.render_context.uniform_buffer_data =
            std::make_shared<vk::Meow::BufferData>(*g_runtime_global_context.render_context.physical_device,
                                                   *g_runtime_global_context.render_context.logical_device,
                                                   sizeof(UBOData),
                                                   vk::BufferUsageFlagBits::eUniformBuffer);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Render System Uniform Buffer";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {
            vk::ObjectType::eBuffer,
            vk::Meow::GetVulkanHandle(*g_runtime_global_context.render_context.uniform_buffer_data->buffer),
            object_name.c_str(),
            nullptr};
        g_runtime_global_context.render_context.logical_device->setDebugUtilsObjectNameEXT(name_info);

        object_name = "Uniform Buffer Device Memory";
        name_info   = {
            vk::ObjectType::eDeviceMemory,
            vk::Meow::GetVulkanHandle(*g_runtime_global_context.render_context.uniform_buffer_data->device_memory),
            object_name.c_str(),
            nullptr};
        g_runtime_global_context.render_context.logical_device->setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void RenderSystem::CreateDescriptorSetLayout()
    {
        g_runtime_global_context.render_context.descriptor_set_layout =
            std::make_shared<vk::raii::DescriptorSetLayout>(vk::Meow::MakeDescriptorSetLayout(
                *g_runtime_global_context.render_context.logical_device,
                {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}}));
    }

    void RenderSystem::CreatePipelineLayout()
    {
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info(
            {}, **g_runtime_global_context.render_context.descriptor_set_layout);
        g_runtime_global_context.render_context.pipeline_layout = std::make_shared<vk::raii::PipelineLayout>(
            *g_runtime_global_context.render_context.logical_device, pipeline_layout_create_info);
    }

    void RenderSystem::CreateDescriptorPool()
    {
        // create a descriptor pool
        vk::DescriptorPoolSize       pool_size(vk::DescriptorType::eUniformBuffer, 1);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, pool_size);
        g_runtime_global_context.render_context.descriptor_pool = std::make_shared<vk::raii::DescriptorPool>(
            *g_runtime_global_context.render_context.logical_device, descriptor_pool_create_info);
    }

    void RenderSystem::CreateDescriptorSet()
    {
        // allocate a descriptor set
        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(
            **g_runtime_global_context.render_context.descriptor_pool,
            **g_runtime_global_context.render_context.descriptor_set_layout);
        g_runtime_global_context.render_context.descriptor_set = std::make_shared<vk::raii::DescriptorSet>(
            std::move(vk::raii::DescriptorSets(*g_runtime_global_context.render_context.logical_device,
                                               descriptor_set_allocate_info)
                          .front()));

        // TODO: Uniform set is temp
        vk::Meow::UpdateDescriptorSets(*g_runtime_global_context.render_context.logical_device,
                                       *g_runtime_global_context.render_context.descriptor_set,
                                       {{vk::DescriptorType::eUniformBuffer,
                                         g_runtime_global_context.render_context.uniform_buffer_data->buffer,
                                         VK_WHOLE_SIZE,
                                         nullptr}},
                                       {});
    }

    void RenderSystem::CreateRenderPass()
    {
        vk::Format color_format =
            vk::Meow::PickSurfaceFormat(
                (*g_runtime_global_context.render_context.physical_device)
                    .getSurfaceFormatsKHR(*g_runtime_global_context.render_context.surface_data->surface))
                .format;

        g_runtime_global_context.render_context.render_pass = std::make_shared<vk::raii::RenderPass>(
            vk::Meow::MakeRenderPass(*g_runtime_global_context.render_context.logical_device,
                                     color_format,
                                     g_runtime_global_context.render_context.depth_buffer_data->format));

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Render System Render Pass";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {
            vk::ObjectType::eRenderPass,
            vk::Meow::GetVulkanHandle(**g_runtime_global_context.render_context.render_pass),
            object_name.c_str(),
            nullptr};
        g_runtime_global_context.render_context.logical_device->setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    VOID RenderSystem::CreateFramebuffers()
    {
        g_runtime_global_context.render_context.framebuffers =
            vk::Meow::MakeFramebuffers(*g_runtime_global_context.render_context.logical_device,
                                       *g_runtime_global_context.render_context.render_pass,
                                       g_runtime_global_context.render_context.swapchain_data->image_views,
                                       &g_runtime_global_context.render_context.depth_buffer_data->image_view,
                                       g_runtime_global_context.render_context.surface_data->extent);
    }

    void RenderSystem::CreatePipeline()
    {
        // --------------Create Shaders--------------

        // TODO: temp Shader

        uint8_t* data_ptr = nullptr;
        uint32_t data_size;

        g_runtime_global_context.file_system.get()->ReadBinaryFile(
            "builtin/shaders/mesh.vert.spv", data_ptr, data_size);
        vk::raii::ShaderModule vertex_shader_module(*g_runtime_global_context.render_context.logical_device,
                                                    {vk::ShaderModuleCreateFlags(), data_size, (uint32_t*)data_ptr});

        delete[] data_ptr;
        data_ptr = nullptr;

        g_runtime_global_context.file_system.get()->ReadBinaryFile(
            "builtin/shaders/mesh.frag.spv", data_ptr, data_size);
        vk::raii::ShaderModule fragment_shader_module(*g_runtime_global_context.render_context.logical_device,
                                                      {vk::ShaderModuleCreateFlags(), data_size, (uint32_t*)data_ptr});

        delete[] data_ptr;
        data_ptr = nullptr;

        // --------------Create Pipeline--------------

        // TODO: temp vertex layout
        vk::raii::PipelineCache pipeline_cache(*g_runtime_global_context.render_context.logical_device,
                                               vk::PipelineCacheCreateInfo());
        g_runtime_global_context.render_context.graphics_pipeline =
            std::make_shared<vk::raii::Pipeline>(vk::Meow::MakeGraphicsPipeline(
                *g_runtime_global_context.render_context.logical_device,
                pipeline_cache,
                vertex_shader_module,
                nullptr,
                fragment_shader_module,
                nullptr,
                VertexAttributesToSize({VertexAttribute::VA_Position, VertexAttribute::VA_Normal}),
                {{vk::Format::eR32G32B32Sfloat, 0}, {vk::Format::eR32G32B32Sfloat, 12}},
                vk::FrontFace::eClockwise,
                true,
                *g_runtime_global_context.render_context.pipeline_layout,
                *g_runtime_global_context.render_context.render_pass));
    }

    void RenderSystem::CreateSyncObjects()
    {
        g_runtime_global_context.render_context.image_acquired_semaphores.resize(vk::Meow::k_max_frames_in_flight);
        g_runtime_global_context.render_context.render_finished_semaphores.resize(vk::Meow::k_max_frames_in_flight);
        g_runtime_global_context.render_context.in_flight_fences.resize(vk::Meow::k_max_frames_in_flight);

        for (size_t i = 0; i < vk::Meow::k_max_frames_in_flight; i++)
        {
            g_runtime_global_context.render_context.image_acquired_semaphores[i] =
                std::make_shared<vk::raii::Semaphore>(*g_runtime_global_context.render_context.logical_device,
                                                      vk::SemaphoreCreateInfo());

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            std::string                     object_name = std::format("Image Acquired Semaphore {}", i);
            vk::DebugUtilsObjectNameInfoEXT name_info   = {
                vk::ObjectType::eSemaphore,
                vk::Meow::GetVulkanHandle(**g_runtime_global_context.render_context.image_acquired_semaphores[i]),
                object_name.c_str(),
                nullptr};
            g_runtime_global_context.render_context.logical_device->setDebugUtilsObjectNameEXT(name_info);
#endif
        }

        for (size_t i = 0; i < vk::Meow::k_max_frames_in_flight; i++)
        {
            g_runtime_global_context.render_context.render_finished_semaphores[i] =
                std::make_shared<vk::raii::Semaphore>(*g_runtime_global_context.render_context.logical_device,
                                                      vk::SemaphoreCreateInfo());

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            std::string                     object_name = std::format("Render Finished Semaphore {}", i);
            vk::DebugUtilsObjectNameInfoEXT name_info   = {
                vk::ObjectType::eSemaphore,
                vk::Meow::GetVulkanHandle(**g_runtime_global_context.render_context.render_finished_semaphores[i]),
                object_name.c_str(),
                nullptr};
            g_runtime_global_context.render_context.logical_device->setDebugUtilsObjectNameEXT(name_info);
#endif
        }

        for (size_t i = 0; i < vk::Meow::k_max_frames_in_flight; i++)
        {
            g_runtime_global_context.render_context.in_flight_fences[i] = std::make_shared<vk::raii::Fence>(
                *g_runtime_global_context.render_context.logical_device, vk::FenceCreateInfo());

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            std::string                     object_name = std::format("In Flight Fence {}", i);
            vk::DebugUtilsObjectNameInfoEXT name_info   = {
                vk::ObjectType::eFence,
                vk::Meow::GetVulkanHandle(**g_runtime_global_context.render_context.in_flight_fences[i]),
                object_name.c_str(),
                nullptr};
            g_runtime_global_context.render_context.logical_device->setDebugUtilsObjectNameEXT(name_info);
#endif
        }
    }

    void RenderSystem::InitImGui()
    {
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
        g_runtime_global_context.render_context.imgui_descriptor_pool = std::make_shared<vk::raii::DescriptorPool>(
            *g_runtime_global_context.render_context.logical_device, descriptor_pool_create_info);

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
        ImGui_ImplGlfw_InitForVulkan(g_runtime_global_context.render_context.window->GetGLFWWindow(), true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = **g_runtime_global_context.render_context.vulkan_instance;
        init_info.PhysicalDevice            = **g_runtime_global_context.render_context.physical_device;
        init_info.Device                    = **g_runtime_global_context.render_context.logical_device;
        init_info.QueueFamily               = g_runtime_global_context.render_context.graphics_queue_family_index;
        init_info.Queue                     = **g_runtime_global_context.render_context.graphics_queue;
        init_info.DescriptorPool            = **g_runtime_global_context.render_context.imgui_descriptor_pool;
        init_info.Subpass                   = 0;
        init_info.MinImageCount             = vk::Meow::k_max_frames_in_flight;
        init_info.ImageCount                = vk::Meow::k_max_frames_in_flight;
        init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;

        ImGui_ImplVulkan_Init(&init_info, **g_runtime_global_context.render_context.render_pass);

        // Upload Fonts
        {
            // Use any command queue
            auto& cmd_buffer = g_runtime_global_context.render_context
                                   .command_buffers[g_runtime_global_context.render_context.current_frame_index];

            g_runtime_global_context.render_context.command_pool->reset();
            cmd_buffer.begin({});

            ImGui_ImplVulkan_CreateFontsTexture(*cmd_buffer);

            vk::SubmitInfo submit_info({}, {}, *cmd_buffer, {});

            cmd_buffer.end();
            g_runtime_global_context.render_context.graphics_queue->submit(submit_info);

            g_runtime_global_context.render_context.logical_device->waitIdle();
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
    }
} // namespace Meow
