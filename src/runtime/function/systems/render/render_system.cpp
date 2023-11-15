#include "render_system.h"

#include "core/log/log.h"
#include "function/components/3d/camera/camera_3d_component.h"
#include "function/components/3d/model/model_component.h"
#include "function/components/3d/transform/transform_3d_component.h"
#include "function/components/shared/pawn_component.h"
#include "function/global/runtime_global_context.h"
#include "function/systems/render/utils/vulkan_initialize_utils.hpp"
#include "function/systems/render/utils/vulkan_update_utils.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/gtc/matrix_transform.hpp>
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
        std::vector<const char*> optimal_validation_layers =
            GetOptimalValidationLayers(supported_validation_layers);
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
            where =
                ranked_devices.insert(where, {ScorePhysicalDevice(gpu, k_required_device_extensions), gpu});
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
        float                     queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_info({}, m_graphics_queue_family_index, 1, &queue_priority);
        vk::DeviceCreateInfo      device_info({}, queue_info, {}, k_required_device_extensions);
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

    void RenderSystem::CreateDepthBuffer()
    {
        m_depth_buffer_data = DepthBufferData(m_gpu, m_logical_device, vk::Format::eD16Unorm, m_surface_data.extent);
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Depth Image";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {vk::ObjectType::eImage,
                                                       GetVulkanHandle(*m_depth_buffer_data.image),
                                                       object_name.c_str(),
                                                       nullptr};
        m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void RenderSystem::CreateUniformBuffer()
    {
        // TODO: UBOData is temp
        m_uniform_buffer_data =
            BufferData(m_gpu, m_logical_device, sizeof(UBOData), vk::BufferUsageFlagBits::eUniformBuffer);
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Uniform Buffer";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {vk::ObjectType::eBuffer,
                                                       GetVulkanHandle(*m_uniform_buffer_data.buffer),
                                                       object_name.c_str(),
                                                       nullptr};
        m_logical_device.setDebugUtilsObjectNameEXT(name_info);
        object_name = "Uniform Buffer Device Memory";
        name_info   = {vk::ObjectType::eDeviceMemory,
                       GetVulkanHandle(*m_uniform_buffer_data.device_memory),
                       object_name.c_str(),
                       nullptr};
        m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif
    }

    void RenderSystem::CreateDescriptorSetLayout()
    {
        m_descriptor_set_layout = MakeDescriptorSetLayout(
            m_logical_device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}});
    }

    void RenderSystem::CreateDescriptorPool()
    {
        // create a descriptor pool
        vk::DescriptorPoolSize       pool_size(vk::DescriptorType::eUniformBuffer, 1);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, pool_size);
        m_descriptor_pool = vk::raii::DescriptorPool(m_logical_device, descriptor_pool_create_info);
    }

    void RenderSystem::CreateDescriptorSet()
    {
        // allocate a descriptor set
        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(*m_descriptor_pool, *m_descriptor_set_layout);
        m_descriptor_set = vk::raii::DescriptorSet(
            std::move(vk::raii::DescriptorSets(m_logical_device, descriptor_set_allocate_info).front()));
        // TODO: Uniform set is temp
        UpdateDescriptorSets(
            m_logical_device,
            m_descriptor_set,
            {{vk::DescriptorType::eUniformBuffer, m_uniform_buffer_data.buffer, VK_WHOLE_SIZE, nullptr}},
            {});
    }

    void RenderSystem::CreateRenderPass()
    {
        vk::Format color_format =
            PickSurfaceFormat((m_gpu).getSurfaceFormatsKHR(*m_surface_data.surface)).format;
        m_render_pass = MakeRenderPass(m_logical_device, color_format, m_depth_buffer_data.format);
    }

    void RenderSystem::CreateFramebuffers()
    {
        m_framebuffers = MakeFramebuffers(m_logical_device,
                                                    m_render_pass,
                                                    m_swapchain_data.image_views,
                                                    &m_depth_buffer_data.image_view,
                                                    m_surface_data.extent);
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

        ImGui_ImplVulkan_Init(&init_info, *m_render_pass);

        // Upload Fonts
        {
            // Use any command queue
            auto& per_frame_data = m_per_frame_data[m_current_frame_index];
            auto& cmd_pool       = per_frame_data.command_pool;
            auto& cmd_buffer     = per_frame_data.command_buffer;

            cmd_pool.reset();
            cmd_buffer.begin({});

            ImGui_ImplVulkan_CreateFontsTexture(*cmd_buffer);

            vk::SubmitInfo submit_info({}, {}, *cmd_buffer, {});

            cmd_buffer.end();
            m_graphics_queue.submit(submit_info);

            m_logical_device.waitIdle();
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
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
        CreateDepthBuffer();
        CreateUniformBuffer();
        CreateDescriptorSetLayout();
        CreateDescriptorPool();
        CreateDescriptorSet();
        CreateRenderPass();
        CreateFramebuffers();
        CreatePerFrameData();
        InitImGui();

        // TODO: temp material creation
        g_runtime_global_context.resource_system->m_materials["Default Material"] =
            Material(m_logical_device, m_descriptor_set_layout, m_render_pass);

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
            m_gpu,
            m_logical_device,
            m_upload_context.command_pool,
            m_graphics_queue,
            "builtin/models/monkey_head.obj",
            std::vector<VertexAttribute> {VertexAttribute::VA_Position, VertexAttribute::VA_Normal});

        BoundingBox model_bounding          = model_component.model.GetBounding();
        glm::vec3   bound_size              = model_bounding.max - model_bounding.min;
        glm::vec3   bound_center            = model_bounding.min + bound_size * 0.5f;
        camera_transform_component.position = bound_center + glm::vec3(0.0f, 0.0f, -50.0f);
        // glm::lookAt
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
    }

    void RenderSystem::UpdateUniformBuffer(UBOData ubo_data)
    {
        CopyToDevice(m_uniform_buffer_data.device_memory, ubo_data);
        UpdateDescriptorSets(
            m_logical_device,
            m_descriptor_set,
            {{vk::DescriptorType::eUniformBuffer, m_uniform_buffer_data.buffer, VK_WHOLE_SIZE, nullptr}},
            {});
    }

    /**
     * @brief Begin command buffer and render pass. Set viewport and scissor.
     */
    bool RenderSystem::StartRenderpass()
    {
        auto& per_frame_data           = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer               = per_frame_data.command_buffer;
        auto& image_acquired_semaphore = per_frame_data.image_acquired_semaphore;

        vk::Result result;
        std::tie(result, m_current_image_index) =
            m_swapchain_data.swap_chain.acquireNextImage(k_fence_timeout, *image_acquired_semaphore);

        assert(result == vk::Result::eSuccess);
        assert(m_current_image_index < m_swapchain_data.images.size());

        cmd_buffer.reset();
        cmd_buffer.begin({});
        cmd_buffer.setViewport(0,
                               vk::Viewport(0.0f,
                                            static_cast<float>(m_surface_data.extent.height),
                                            static_cast<float>(m_surface_data.extent.width),
                                            -static_cast<float>(m_surface_data.extent.height),
                                            0.0f,
                                            1.0f));
        cmd_buffer.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_surface_data.extent));
        std::array<vk::ClearValue, 2> clear_values;
        clear_values[0].color        = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 0.2f);
        clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderPassBeginInfo render_pass_begin_info(*m_render_pass,
                                                       *m_framebuffers[m_current_image_index],
                                                       vk::Rect2D(vk::Offset2D(0, 0), m_surface_data.extent),
                                                       clear_values);
        cmd_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
        return true;
    }

    /**
     * @brief End render pass and command buffer. Submit graphics queue. Present.
     */
    void RenderSystem::EndRenderpass()
    {
        auto& per_frame_data            = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer                = per_frame_data.command_buffer;
        auto& image_acquired_semaphore  = per_frame_data.image_acquired_semaphore;
        auto& render_finished_semaphore = per_frame_data.render_finished_semaphore;
        auto& in_flight_fence           = per_frame_data.in_flight_fence;

        cmd_buffer.endRenderPass();
        cmd_buffer.end();

        vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo         submit_info(
            *image_acquired_semaphore, wait_destination_stage_mask, *cmd_buffer, *render_finished_semaphore);
        m_graphics_queue.submit(submit_info, *in_flight_fence);

        while (vk::Result::eTimeout == m_logical_device.waitForFences({*in_flight_fence}, VK_TRUE, k_fence_timeout))
            ;
        m_logical_device.resetFences({*in_flight_fence});

        vk::PresentInfoKHR present_info(
            *render_finished_semaphore, *m_swapchain_data.swap_chain, m_current_image_index);
        vk::Result result = m_present_queue.presentKHR(present_info);
        switch (result)
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
                glm::ivec2 window_size = g_runtime_global_context.window_system->m_window->GetSize();

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

        StartRenderpass();

        auto& per_frame_data = m_per_frame_data[m_current_frame_index];
        auto& cmd_buffer     = per_frame_data.command_buffer;

        std::unordered_map<Material*, std::vector<Renderable*>> renderables_per_material;

        for (auto [entity, transfrom_component, model_component] :
             g_runtime_global_context.registry.view<const Transform3DComponent, ModelComponent>().each())
        {
            // TODO: How to solve the different uniform buffer problem?
            // ubo_data.model = transfrom_component.m_global_transform;

            for (Mesh& mesh : model_component.model.meshes)
            {
                Material* material_ptr = &g_runtime_global_context.resource_system->m_materials[mesh.material_name];
                renderables_per_material[material_ptr].push_back(&mesh);
            }
        }

        // TODO: sort renderables by distance

        for (std::pair<Material*, std::vector<Renderable*>> kv : renderables_per_material)
        {
            kv.first->Bind(cmd_buffer, m_descriptor_set);

            for (Renderable* renderable : kv.second)
            {
                renderable->BindDrawCmd(cmd_buffer);
            }
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

        EndRenderpass();
    }
} // namespace Meow
