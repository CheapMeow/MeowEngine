#include "render_system.h"

#include "core/log/log.h"
#include "function/components/3d/camera/camera_3d_component.h"
#include "function/components/3d/model/model_component.h"
#include "function/components/3d/transform/transform_3d_component.h"
#include "function/global/runtime_global_context.h"
#include "function/render/utils/vulkan_initialize_utils.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <volk.h>

namespace Meow
{
    void RenderSystem::CreateVulkanInstance()
    {
#ifdef MEOW_DEBUG
        LogVulkanAPIVersion(m_vulkan_context.enumerateInstanceVersion());
#endif

        // prepare for create vk::InstanceCreateInfo
        std::vector<vk::ExtensionProperties> available_instance_extensions =
            m_vulkan_context.enumerateInstanceExtensionProperties();
        std::vector<const char*> required_instance_extensions =
            GetRequiredInstanceExtensions({VK_KHR_SURFACE_EXTENSION_NAME});
        if (!ValidateExtensions(required_instance_extensions, available_instance_extensions))
        {
            throw std::runtime_error("Required instance extensions are missing.");
        }

        std::vector<vk::LayerProperties> supported_validation_layers =
            m_vulkan_context.enumerateInstanceLayerProperties();
        std::vector<const char*> required_validation_layers {};
#ifdef VKB_VALIDATION_LAYERS
        // Determine the optimal validation layers to enable that are necessary for useful debugging
        std::vector<const char*> optimal_validation_layers = GetOptimalValidationLayers(supported_validation_layers);
        required_validation_layers.insert(
            required_validation_layers.end(), optimal_validation_layers.begin(), optimal_validation_layers.end());
#endif
        if (ValidateLayers(required_validation_layers, supported_validation_layers))
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

        uint32_t               api_version = m_vulkan_context.enumerateInstanceVersion();
        vk::ApplicationInfo    app("Meow Engine Vulkan Renderer", {}, "Meow Engine", {}, api_version);
        vk::InstanceCreateInfo instance_info({}, &app, required_validation_layers, required_instance_extensions);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        // VkDebugUtilsMessengerEXT only covers stuff from its creation to its destruction.
        // vkCreateInstance and vkDestroyInstance are covered by the special pNext variant
        // because at that point the VkDebugUtilsMessengerEXT object cannot even exist yet\anymore
        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info = MakeDebugUtilsMessengerCreateInfoEXT();
        instance_info.pNext                                          = &debug_utils_create_info;
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
        instance_info.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

        m_vulkan_instance = vk::raii::Instance(m_vulkan_context, instance_info);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        // we need some additional initializing for this platform!
        if (volkInitialize())
        {
            throw std::runtime_error("Failed to initialize volk.");
        }
        volkLoadInstance(instance);
#endif
    }

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
    void RenderSystem::CreateDebugUtilsMessengerEXT()
    {
        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info = MakeDebugUtilsMessengerCreateInfoEXT();
        m_debug_utils_messenger = vk::raii::DebugUtilsMessengerEXT(m_vulkan_instance, debug_utils_create_info);
    }
#endif

    void RenderSystem::CreatePhysicalDevice()
    {
        vk::raii::PhysicalDevices gpus(m_vulkan_instance);

        // Maps to hold devices and sort by rank.
        std::multimap<uint32_t, vk::raii::PhysicalDevice> ranked_devices;
        auto                                              where = ranked_devices.end();
        // Iterates through all devices and rate their suitability.
        for (const auto& gpu : gpus)
            where = ranked_devices.insert(where, {ScorePhysicalDevice(gpu, k_required_device_extensions), gpu});
        // Checks to make sure the best candidate scored higher than 0  rbegin points to last element of ranked
        // devices(highest rated), first is its rating.
        if (ranked_devices.rbegin()->first < 0)
        {
            throw std::runtime_error("Best gpu get negitive score.");
        }

        m_gpu = vk::raii::PhysicalDevice(ranked_devices.rbegin()->second);
    }

    void RenderSystem::CreateSurface()
    {
        auto         size = g_runtime_global_context.window_system.get()->m_window->GetSize();
        vk::Extent2D extent(size.x, size.y);
        m_surface_data = SurfaceData(
            m_vulkan_instance, g_runtime_global_context.window_system.get()->m_window->GetGLFWWindow(), extent);
    }

    void RenderSystem::CreateLogicalDevice()
    {
        std::vector<vk::ExtensionProperties> device_extensions = m_gpu.enumerateDeviceExtensionProperties();
        if (!ValidateExtensions(k_required_device_extensions, device_extensions))
        {
            throw std::runtime_error("Required device extensions are missing, will try without.");
        }

        auto indexs                   = FindGraphicsAndPresentQueueFamilyIndex(m_gpu, m_surface_data.surface);
        m_graphics_queue_family_index = indexs.first;
        m_present_queue_family_index  = indexs.second;

        // Create a device with one queue
        float                      queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo  queue_info({}, m_graphics_queue_family_index, 1, &queue_priority);
        vk::PhysicalDeviceFeatures physical_device_feature;
        physical_device_feature.pipelineStatisticsQuery = vk::True;

        vk::DeviceCreateInfo device_info({},                           /* flags */
                                         queue_info,                   /* queueCreateInfoCount */
                                         {},                           /* ppEnabledLayerNames */
                                         k_required_device_extensions, /* ppEnabledExtensionNames */
                                         &physical_device_feature);    /* pEnabledFeatures */
        m_logical_device = vk::raii::Device(m_gpu, device_info);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        volkLoadDevice(*logical_device);
#endif

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Logical Device";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {
            vk::ObjectType::eDevice, GetVulkanHandle(*m_logical_device), object_name.c_str(), nullptr};
        m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

        m_graphics_queue = vk::raii::Queue(m_logical_device, m_graphics_queue_family_index, 0);
        m_present_queue  = vk::raii::Queue(m_logical_device, m_present_queue_family_index, 0);
    }

    void RenderSystem::CreateSwapChian()
    {
        m_swapchain_data =
            SwapChainData(m_gpu,
                          m_logical_device,
                          m_surface_data.surface,
                          m_surface_data.extent,
                          vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                          nullptr,
                          m_graphics_queue_family_index,
                          m_present_queue_family_index);
    }

    void RenderSystem::CreateUploadContext()
    {
        vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eTransient |
                                                               vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                           m_graphics_queue_family_index);
        m_upload_context.command_pool = vk::raii::CommandPool(m_logical_device, command_pool_create_info);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Command pool in UploadContext";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {vk::ObjectType::eCommandPool,
                                                       GetVulkanHandle(*m_upload_context.command_pool),
                                                       object_name.c_str(),
                                                       nullptr};
        m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void RenderSystem::CreateDescriptorAllocator()
    {
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
        m_descriptor_allocator = DescriptorAllocatorGrowable(m_logical_device, 1000, pool_sizes);
    }

    void RenderSystem::CreatePerFrameData()
    {
        m_per_frame_data.resize(k_max_frames_in_flight);

        for (uint32_t i = 0; i < k_max_frames_in_flight; ++i)
        {
            vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eTransient |
                                                                   vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                               m_graphics_queue_family_index);
            m_per_frame_data[i].command_pool = vk::raii::CommandPool(m_logical_device, command_pool_create_info);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            std::string                     object_name = std::format("Command Pool in PerFrameData {}", i);
            vk::DebugUtilsObjectNameInfoEXT name_info   = {vk::ObjectType::eCommandPool,
                                                           GetVulkanHandle(*m_per_frame_data[i].command_pool),
                                                           object_name.c_str(),
                                                           nullptr};
            m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

            vk::CommandBufferAllocateInfo command_buffer_allocate_info(
                *m_per_frame_data[i].command_pool, vk::CommandBufferLevel::ePrimary, 1);
            vk::raii::CommandBuffers command_buffers(m_logical_device, command_buffer_allocate_info);
            m_per_frame_data[i].command_buffer = std::move(command_buffers[0]);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            object_name = std::format("Command Buffer in PerFrameData {}", i);
            name_info   = {vk::ObjectType::eCommandBuffer,
                           GetVulkanHandle(*m_per_frame_data[i].command_buffer),
                           object_name.c_str(),
                           nullptr};
            m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

            m_per_frame_data[i].image_acquired_semaphore =
                vk::raii::Semaphore(m_logical_device, vk::SemaphoreCreateInfo());

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            object_name = std::format("Image Acquired Semaphore in PerFrameData {}", i);
            name_info   = {vk::ObjectType::eSemaphore,
                           GetVulkanHandle(*m_per_frame_data[i].image_acquired_semaphore),
                           object_name.c_str(),
                           nullptr};
            m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

            m_per_frame_data[i].render_finished_semaphore =
                vk::raii::Semaphore(m_logical_device, vk::SemaphoreCreateInfo());

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            object_name = std::format("Render Finished Semaphore in PerFrameData {}", i);
            name_info   = {vk::ObjectType::eSemaphore,
                           GetVulkanHandle(*m_per_frame_data[i].render_finished_semaphore),
                           object_name.c_str(),
                           nullptr};
            m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

            m_per_frame_data[i].in_flight_fence = vk::raii::Fence(m_logical_device, vk::FenceCreateInfo());

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            object_name = std::format("In Flight Fence in PerFrameData{}", i);
            name_info   = {vk::ObjectType::eFence,
                           GetVulkanHandle(*m_per_frame_data[i].in_flight_fence),
                           object_name.c_str(),
                           nullptr};
            m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
        }
    }

    void RenderSystem::CreateRenderPass()
    {
        auto& per_frame_data = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer     = per_frame_data.command_buffer;

        m_deferred_pass = DeferredPass(m_gpu,
                                       m_logical_device,
                                       m_surface_data,
                                       m_upload_context.command_pool,
                                       m_graphics_queue,
                                       m_descriptor_allocator);
        m_deferred_pass.RefreshFrameBuffers(
            m_gpu, m_logical_device, cmd_buffer, m_surface_data, m_swapchain_data.image_views, m_surface_data.extent);

        m_forward_pass = ForwardPass(m_gpu,
                                     m_logical_device,
                                     m_surface_data,
                                     m_upload_context.command_pool,
                                     m_graphics_queue,
                                     m_descriptor_allocator);
        m_forward_pass.RefreshFrameBuffers(
            m_gpu, m_logical_device, cmd_buffer, m_surface_data, m_swapchain_data.image_views, m_surface_data.extent);

        m_imgui_pass = ImguiPass(m_gpu,
                                 m_logical_device,
                                 m_surface_data,
                                 m_upload_context.command_pool,
                                 m_graphics_queue,
                                 m_descriptor_allocator);
        m_imgui_pass.RefreshFrameBuffers(
            m_gpu, m_logical_device, cmd_buffer, m_surface_data, m_swapchain_data.image_views, m_surface_data.extent);

        m_render_pass_ptr = &m_deferred_pass;
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
        m_imgui_descriptor_pool = vk::raii::DescriptorPool(m_logical_device, descriptor_pool_create_info);

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

        ImGui_ImplGlfw_InitForVulkan(g_runtime_global_context.window_system->m_window->GetGLFWWindow(), true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance                  = *m_vulkan_instance;
        init_info.PhysicalDevice            = *m_gpu;
        init_info.Device                    = *m_logical_device;
        init_info.QueueFamily               = m_graphics_queue_family_index;
        init_info.Queue                     = *m_graphics_queue;
        init_info.DescriptorPool            = *m_imgui_descriptor_pool;
        init_info.Subpass                   = 0;
        init_info.MinImageCount             = k_max_frames_in_flight;
        init_info.ImageCount                = k_max_frames_in_flight;
        init_info.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        init_info.RenderPass                = *m_imgui_pass.render_pass;

        ImGui_ImplVulkan_Init(&init_info);

        // Upload Fonts
        {
            // Use any command queue
            auto& per_frame_data = m_per_frame_data[m_current_frame_index];
            auto& cmd_pool       = per_frame_data.command_pool;
            auto& cmd_buffer     = per_frame_data.command_buffer;

            cmd_pool.reset();
            cmd_buffer.begin({});

            ImGui_ImplVulkan_CreateFontsTexture();

            vk::SubmitInfo submit_info({}, {}, *cmd_buffer, {});

            cmd_buffer.end();
            m_graphics_queue.submit(submit_info);

            m_logical_device.waitIdle();
        }
    }

    void RenderSystem::RecreateSwapChain()
    {
        auto& per_frame_data = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer     = per_frame_data.command_buffer;

        m_logical_device.waitIdle();

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
        m_deferred_pass.RefreshFrameBuffers(
            m_gpu, m_logical_device, cmd_buffer, m_surface_data, m_swapchain_data.image_views, m_surface_data.extent);
        m_forward_pass.RefreshFrameBuffers(
            m_gpu, m_logical_device, cmd_buffer, m_surface_data, m_swapchain_data.image_views, m_surface_data.extent);
        m_imgui_pass.RefreshFrameBuffers(
            m_gpu, m_logical_device, cmd_buffer, m_surface_data, m_swapchain_data.image_views, m_surface_data.extent);
    }

    RenderSystem::RenderSystem()
    {
        CreateVulkanInstance();
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        CreateDebugUtilsMessengerEXT();
#endif
        CreatePhysicalDevice();
        CreateSurface();
        CreateLogicalDevice();
        CreateSwapChian();
        CreateUploadContext();
        CreateDescriptorAllocator();
        CreatePerFrameData();
        CreateRenderPass();
        InitImGui();
    }

    RenderSystem::~RenderSystem()
    {
        m_logical_device.waitIdle();

        // TODO: deleting entity should be obstract
        auto model_view = g_runtime_global_context.registry.view<Transform3DComponent, ModelComponent>();
        g_runtime_global_context.registry.destroy(model_view.begin(), model_view.end());

        auto camera_view = g_runtime_global_context.registry.view<Transform3DComponent, Camera3DComponent>();
        g_runtime_global_context.registry.destroy(camera_view.begin(), camera_view.end());

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_imgui_pass            = nullptr;
        m_imgui_descriptor_pool = nullptr;
        m_per_frame_data.clear();
        m_forward_pass         = nullptr;
        m_deferred_pass        = nullptr;
        m_descriptor_allocator = nullptr;
        m_upload_context       = nullptr;
        m_swapchain_data       = nullptr;
        m_present_queue        = nullptr;
        m_graphics_queue       = nullptr;
        m_logical_device       = nullptr;
        m_surface_data         = nullptr;
        m_gpu                  = nullptr;
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        m_debug_utils_messenger = nullptr;
#endif
        m_vulkan_instance = nullptr;
    }

    void RenderSystem::Start()
    {
        auto& per_frame_data = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer     = per_frame_data.command_buffer;

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
        auto& model_component =
            g_runtime_global_context.registry.emplace<ModelComponent>(model_entity,
                                                                      m_gpu,
                                                                      m_logical_device,
                                                                      m_upload_context.command_pool,
                                                                      m_graphics_queue,
                                                                      "builtin/models/backpack/backpack.obj",
                                                                      m_render_pass_ptr->input_vertex_attributes);

        BoundingBox model_bounding          = model_component.model.root_node->GetBounds();
        glm::vec3   bound_size              = model_bounding.max - model_bounding.min;
        glm::vec3   bound_center            = model_bounding.min + bound_size * 0.5f;
        camera_transform_component.position = bound_center + glm::vec3(0.0f, 0.0f, -25.0f);
        // glm::lookAt
    }

    void RenderSystem::Update(float frame_time)
    {
        auto& per_frame_data            = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer                = per_frame_data.command_buffer;
        auto& image_acquired_semaphore  = per_frame_data.image_acquired_semaphore;
        auto& render_finished_semaphore = per_frame_data.render_finished_semaphore;
        auto& in_flight_fence           = per_frame_data.in_flight_fence;

        // switch render pass
        if (cur_render_pass == 0)
            m_render_pass_ptr = &m_deferred_pass;
        else if (cur_render_pass == 1)
            m_render_pass_ptr = &m_forward_pass;

        m_render_pass_ptr->UpdateUniformBuffer();

        // ------------------- render -------------------

        vk::Result result;
        std::tie(result, m_current_image_index) =
            m_swapchain_data.swap_chain.acquireNextImage(k_fence_timeout, *image_acquired_semaphore);

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
        m_graphics_queue.submit(submit_info, *in_flight_fence);

        while (vk::Result::eTimeout == m_logical_device.waitForFences({*in_flight_fence}, VK_TRUE, k_fence_timeout))
            ;
        cmd_buffer.reset();
        m_logical_device.resetFences({*in_flight_fence});

        vk::PresentInfoKHR present_info(
            *render_finished_semaphore, *m_swapchain_data.swap_chain, m_current_image_index);
        vk::Result result2 = m_present_queue.presentKHR(present_info);
        switch (result2)
        {
            case vk::Result::eSuccess:
                break;
            case vk::Result::eSuboptimalKHR:
                RUNTIME_ERROR("vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n");
                break;
            default:
                assert(false); // an unexpected result is returned !
        }

        m_current_frame_index = (m_current_frame_index + 1) % k_max_frames_in_flight;

        m_render_pass_ptr->AfterRenderPass();
    }

    std::shared_ptr<ImageData> RenderSystem::CreateTextureFromFile(const std::string& filepath)
    {
        auto& per_frame_data = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer     = per_frame_data.command_buffer;

        std::shared_ptr<ImageData> texture_ptr =
            ImageData::CreateTextureFromFile(m_gpu, m_logical_device, cmd_buffer, filepath);

        return texture_ptr;
    }
} // namespace Meow
