#include "vulkan_renderer.h"
#include "core/log/log.h"
#include "function/global/runtime_global_context.h"
#include "function/renderer/utils/geometries.hpp"
#include "function/renderer/utils/vulkan_math_utils.hpp"
#include "function/renderer/utils/vulkan_shader_utils.hpp"

#include "SPIRV/GlslangToSpv.h"
#include <map>

namespace Meow
{
    struct UBOData
    {
        glm::mat4 model      = glm::mat4(1.0f);
        glm::mat4 view       = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
    };

    vk::raii::Context VulkanRenderer::CreateVulkanContent()
    {
        vk::raii::Context vulkan_context;

#ifdef MEOW_DEBUG
        vk::Meow::LogVulkanAPIVersion(vulkan_context.enumerateInstanceVersion());
#endif

        return vulkan_context;
    }

    vk::raii::Instance VulkanRenderer::CreateVulkanInstance()
    {
        // prepare for create vk::InstanceCreateInfo

        std::vector<vk::ExtensionProperties> available_instance_extensions =
            m_vulkan_context.enumerateInstanceExtensionProperties();

        std::vector<const char*> required_instance_extensions =
            vk::Meow::GetRequiredInstanceExtensions({VK_KHR_SURFACE_EXTENSION_NAME});

        if (!vk::Meow::ValidateExtensions(required_instance_extensions, available_instance_extensions))
        {
            throw std::runtime_error("Required instance extensions are missing.");
        }

        std::vector<vk::LayerProperties> supported_validation_layers =
            m_vulkan_context.enumerateInstanceLayerProperties();

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

        uint32_t api_version = m_vulkan_context.enumerateInstanceVersion();

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

        vk::raii::Instance vulkan_instance(m_vulkan_context, instance_info);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        // we need some additional initializing for this platform!
        if (volkInitialize())
        {
            throw std::runtime_error("Failed to initialize volk.");
        }
        volkLoadInstance(instance);
#endif

        return vulkan_instance;
    }

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
    vk::raii::DebugUtilsMessengerEXT VulkanRenderer::CreateDebugUtilsMessengerEXT()
    {
        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info = vk::Meow::MakeDebugUtilsMessengerCreateInfoEXT();
        return vk::raii::DebugUtilsMessengerEXT(m_vulkan_instance, debug_utils_create_info);
    }
#endif

    vk::raii::PhysicalDevice VulkanRenderer::CreatePhysicalDevice()
    {
        vk::raii::PhysicalDevices gpus(m_vulkan_instance);

        // Maps to hold devices and sort by rank.
        std::multimap<uint32_t, vk::raii::PhysicalDevice> ranked_devices;
        auto                                              where = ranked_devices.end();

        // Iterates through all devices and rate their suitability.
        for (const auto& gpu : gpus)
            where = ranked_devices.insert(
                where, {vk::Meow::ScorePhysicalDevice(gpu, vk::Meow::k_required_device_extensions), gpu});

        // Checks to make sure the best candidate scored higher than 0  rbegin points to last element of ranked
        // devices(highest rated), first is its rating.
        if (ranked_devices.rbegin()->first < 0)
        {
            throw std::runtime_error("Best gpu get negitive score.");
        }

        vk::raii::PhysicalDevice gpu(ranked_devices.rbegin()->second);

        return gpu;
    }

    vk::Meow::SurfaceData VulkanRenderer::CreateSurface()
    {
        auto         size = g_runtime_global_context.m_window->GetSize();
        vk::Extent2D extent(size.x, size.y);

        vk::Meow::SurfaceData surface_data(
            m_vulkan_instance, g_runtime_global_context.m_window->GetGLFWWindow(), extent);

        return surface_data;
    }

    vk::raii::Device VulkanRenderer::CreateLogicalDevice()
    {
        std::vector<vk::ExtensionProperties> device_extensions = m_gpu.enumerateDeviceExtensionProperties();

        if (!vk::Meow::ValidateExtensions(vk::Meow::k_required_device_extensions, device_extensions))
        {
            throw std::runtime_error("Required device extensions are missing, will try without.");
        }

        auto indexs                   = vk::Meow::FindGraphicsAndPresentQueueFamilyIndex(m_gpu, m_surface_data.surface);
        m_graphics_queue_family_index = indexs.first;
        m_present_queue_family_index  = indexs.second;

        // Create a device with one queue
        float                     queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_info({}, m_graphics_queue_family_index, 1, &queue_priority);
        vk::DeviceCreateInfo      device_info({}, queue_info, {}, vk::Meow::k_required_device_extensions);

        vk::raii::Device logical_device(m_gpu, device_info);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        volkLoadDevice(*logical_device);
#endif

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::DebugUtilsObjectNameInfoEXT name_info = {
            vk::ObjectType::eDevice, vk::Meow::GetVulkanHandle(*logical_device), "Logical Device", nullptr};
        logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

        return logical_device;
    }

    vk::raii::CommandPool VulkanRenderer::CreateCommandPool()
    {
        vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eTransient |
                                                               vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                           m_graphics_queue_family_index);
        vk::raii::CommandPool     command_pool(m_logical_device, command_pool_create_info);

        return command_pool;
    }

    std::vector<vk::raii::CommandBuffer> VulkanRenderer::CreateCommandBuffers()
    {
        vk::CommandBufferAllocateInfo command_buffer_allocate_info(
            *m_command_pool, vk::CommandBufferLevel::ePrimary, vk::Meow::k_max_frames_in_flight);

        return vk::raii::CommandBuffers(m_logical_device, command_buffer_allocate_info);
    }

    vk::Meow::SwapChainData VulkanRenderer::CreateSwapChian()
    {
        return vk::Meow::SwapChainData(m_gpu,
                                       m_logical_device,
                                       m_surface_data.surface,
                                       m_surface_data.extent,
                                       vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                       nullptr,
                                       m_graphics_queue_family_index,
                                       m_present_queue_family_index);
    }

    vk::Meow::DepthBufferData VulkanRenderer::CreateDepthBuffer()
    {
        vk::Meow::DepthBufferData depth_buffer_data(
            m_gpu, m_logical_device, vk::Format::eD16Unorm, m_surface_data.extent);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::DebugUtilsObjectNameInfoEXT name_info = {
            vk::ObjectType::eImage, vk::Meow::GetVulkanHandle(*depth_buffer_data.image), "Depth Image", nullptr};
        m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

        return depth_buffer_data;
    }

    vk::Meow::BufferData VulkanRenderer::CreateUniformBuffer()
    {
        // TODO: UBOData is temp
        vk::Meow::BufferData uniform_buffer_data(
            m_gpu, m_logical_device, sizeof(UBOData), vk::BufferUsageFlagBits::eUniformBuffer);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::DebugUtilsObjectNameInfoEXT name_info = {
            vk::ObjectType::eBuffer, vk::Meow::GetVulkanHandle(*uniform_buffer_data.buffer), "Uniform Buffer", nullptr};
        m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

        return uniform_buffer_data;
    }

    vk::raii::DescriptorSetLayout VulkanRenderer::CreateDescriptorSetLayout()
    {
        return vk::Meow::MakeDescriptorSetLayout(
            m_logical_device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}});
    }

    vk::raii::PipelineLayout VulkanRenderer::CreatePipelineLayout()
    {
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, *m_descriptor_set_layout);
        return vk::raii::PipelineLayout(m_logical_device, pipeline_layout_create_info);
    }

    vk::raii::DescriptorPool VulkanRenderer::CreateDescriptorPool()
    {
        // create a descriptor pool
        vk::DescriptorPoolSize       pool_size(vk::DescriptorType::eUniformBuffer, 1);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, pool_size);
        return vk::raii::DescriptorPool(m_logical_device, descriptor_pool_create_info);
    }

    vk::raii::DescriptorSet VulkanRenderer::CreateDescriptorSet()
    {
        // allocate a descriptor set
        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(*m_descriptor_pool, *m_descriptor_set_layout);
        vk::raii::DescriptorSet       descriptor_set(
            std::move(vk::raii::DescriptorSets(m_logical_device, descriptor_set_allocate_info).front()));

        // TODO: Uniform set is temp
        vk::Meow::UpdateDescriptorSets(
            m_logical_device,
            descriptor_set,
            {{vk::DescriptorType::eUniformBuffer, m_uniform_buffer_data.buffer, VK_WHOLE_SIZE, nullptr}},
            {});

        return descriptor_set;
    }

    vk::raii::RenderPass VulkanRenderer::CreateRenderPass()
    {
        vk::Format color_format =
            vk::Meow::PickSurfaceFormat((m_gpu).getSurfaceFormatsKHR(*m_surface_data.surface)).format;
        return vk::Meow::MakeRenderPass(m_logical_device, color_format, m_depth_buffer_data.format);
    }

    std::vector<vk::raii::Framebuffer> VulkanRenderer::CreateFramebuffers()
    {
        return std::vector<vk::raii::Framebuffer>(std::move(vk::Meow::MakeFramebuffers(m_logical_device,
                                                                                       m_render_pass,
                                                                                       m_swapchain_data.image_views,
                                                                                       &m_depth_buffer_data.image_view,
                                                                                       m_surface_data.extent)));
    }

    vk::Meow::BufferData VulkanRenderer::CreateVertexBuffer()
    {
        // TODO: Remove temp vertex data
        vk::Meow::BufferData vertex_buffer_data(
            m_gpu, m_logical_device, sizeof(ColoredCubeData), vk::BufferUsageFlagBits::eVertexBuffer);
        vk::Meow::CopyToDevice(
            vertex_buffer_data.device_memory, ColoredCubeData, sizeof(ColoredCubeData) / sizeof(ColoredCubeData[0]));

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::DebugUtilsObjectNameInfoEXT name_info = {
            vk::ObjectType::eBuffer, vk::Meow::GetVulkanHandle(*vertex_buffer_data.buffer), "Vertex Buffer", nullptr};
        m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

        return vertex_buffer_data;
    }

    vk::raii::Pipeline VulkanRenderer::CreatePipeline()
    {
        // --------------Create Shaders--------------

        // TODO: temp Shader

        uint8_t* data_ptr = nullptr;
        uint32_t data_size;

        g_runtime_global_context.m_file_system.get()->ReadBinaryFile(
            "builtin/shaders/mesh.vert.spv", data_ptr, data_size);
        vk::raii::ShaderModule vertex_shader_module(m_logical_device,
                                                    {vk::ShaderModuleCreateFlags(), data_size, (uint32_t*)data_ptr});

        delete[] data_ptr;
        data_ptr = nullptr;

        g_runtime_global_context.m_file_system.get()->ReadBinaryFile(
            "builtin/shaders/mesh.frag.spv", data_ptr, data_size);
        vk::raii::ShaderModule fragment_shader_module(m_logical_device,
                                                      {vk::ShaderModuleCreateFlags(), data_size, (uint32_t*)data_ptr});

        delete[] data_ptr;
        data_ptr = nullptr;

        // --------------Create Pipeline--------------

        // TODO: temp vertex layout
        vk::raii::PipelineCache pipeline_cache(m_logical_device, vk::PipelineCacheCreateInfo());
        return vk::Meow::MakeGraphicsPipeline(m_logical_device,
                                              pipeline_cache,
                                              vertex_shader_module,
                                              nullptr,
                                              fragment_shader_module,
                                              nullptr,
                                              vk::Meow::checked_cast<uint32_t>(sizeof(ColoredCubeData[0])),
                                              {{vk::Format::eR32G32B32Sfloat, 0}, {vk::Format::eR32G32B32Sfloat, 12}},
                                              vk::FrontFace::eClockwise,
                                              true,
                                              m_pipeline_layout,
                                              m_render_pass);
    }

    void VulkanRenderer::CreateSyncObjects()
    {
        m_image_acquired_semaphores.resize(vk::Meow::k_max_frames_in_flight);
        m_render_finished_semaphores.resize(vk::Meow::k_max_frames_in_flight);
        m_in_flight_fences.resize(vk::Meow::k_max_frames_in_flight);

        for (size_t i = 0; i < vk::Meow::k_max_frames_in_flight; i++)
        {
            m_image_acquired_semaphores[i] =
                std::make_shared<vk::raii::Semaphore>(m_logical_device, vk::SemaphoreCreateInfo());
            m_render_finished_semaphores[i] =
                std::make_shared<vk::raii::Semaphore>(m_logical_device, vk::SemaphoreCreateInfo());
            m_in_flight_fences[i] = std::make_shared<vk::raii::Fence>(m_logical_device, vk::FenceCreateInfo());
        }
    }

    VulkanRenderer::VulkanRenderer()
        : m_vulkan_context(std::move(CreateVulkanContent()))
        , m_vulkan_instance(std::move(CreateVulkanInstance()))
        , m_gpu(std::move(CreatePhysicalDevice()))
        , m_surface_data(std::move(CreateSurface()))
        , m_logical_device(std::move(CreateLogicalDevice()))
        , m_graphics_queue(m_logical_device, m_graphics_queue_family_index, 0)
        , m_present_queue(m_logical_device, m_present_queue_family_index, 0)
        , m_command_pool(std::move(CreateCommandPool()))
        , m_command_buffers(CreateCommandBuffers())
        , m_swapchain_data(std::move(CreateSwapChian()))
        , m_depth_buffer_data(std::move(CreateDepthBuffer()))
        , m_uniform_buffer_data(std::move(CreateUniformBuffer()))
        , m_descriptor_set_layout(std::move(CreateDescriptorSetLayout()))
        , m_pipeline_layout(std::move(CreatePipelineLayout()))
        , m_descriptor_pool(std::move(CreateDescriptorPool()))
        , m_descriptor_set(std::move(CreateDescriptorSet()))
        , m_render_pass(std::move(CreateRenderPass()))
        , m_framebuffers(CreateFramebuffers())
        , m_vertex_buffer_data(std::move(CreateVertexBuffer()))
        , m_graphics_pipeline(std::move(CreatePipeline()))
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        , m_debug_utils_messenger(std::move(CreateDebugUtilsMessengerEXT()))
#endif
    {
        CreateSyncObjects();

        // TODO: temp
        m_model = new Model("builtin/models/monkey_head.obj",
                            m_gpu,
                            m_logical_device,
                            {VertexAttribute::VA_Position, VertexAttribute::VA_Normal});

        // Update Uniform Buffer
        // TODO: Create a Camera
        UBOData    ubo_data;
        glm::ivec2 window_size = g_runtime_global_context.m_window->GetSize();

        BoundingBox bounding     = m_model->GetBounding();
        glm::vec3   bound_size   = bounding.max - bounding.min;
        glm::vec3   bound_center = bounding.min + bound_size * 0.5f;

        glm::mat4 model_matrix(1.0f);
        model_matrix = glm::translate(model_matrix, bound_center);
        // model_matrix = glm::scale(model_matrix, Scale);
        // model_matrix = glm::rotate(model_matrix, rotAngle, Rotation);
        ubo_data.model      = model_matrix;
        ubo_data.projection = glm::perspectiveFovRH_ZO(
            glm::pi<float>() / 4.0f, (float)window_size[0], (float)window_size[1], 0.1f, 1000.0f);
        vk::Meow::CopyToDevice(m_uniform_buffer_data.device_memory, ubo_data);

        // TODO: temp
        m_camera_position = bound_center;
    }

    /**
     * @brief In order to support move ctor.
     */
    VulkanRenderer::VulkanRenderer(VulkanRenderer&& rhs) VULKAN_HPP_NOEXCEPT
        : m_vulkan_context(std::move(rhs.m_vulkan_context)),
          m_vulkan_instance(std::move(rhs.m_vulkan_instance)),
          m_gpu(std::move(rhs.m_gpu)),
          m_surface_data(std::move(rhs.m_surface_data)),
          m_graphics_queue_family_index(rhs.m_graphics_queue_family_index),
          m_present_queue_family_index(rhs.m_present_queue_family_index),
          m_logical_device(std::move(rhs.m_logical_device)),
          m_graphics_queue(std::move(rhs.m_graphics_queue)),
          m_present_queue(std::move(rhs.m_present_queue)),
          m_command_pool(std::move(rhs.m_command_pool)),
          m_command_buffers(std::move(rhs.m_command_buffers)),
          m_swapchain_data(std::move(rhs.m_swapchain_data)),
          m_depth_buffer_data(std::move(rhs.m_depth_buffer_data)),
          m_uniform_buffer_data(std::move(rhs.m_uniform_buffer_data)),
          m_descriptor_set_layout(std::move(rhs.m_descriptor_set_layout)),
          m_pipeline_layout(std::move(rhs.m_pipeline_layout)),
          m_descriptor_pool(std::move(rhs.m_descriptor_pool)),
          m_descriptor_set(std::move(rhs.m_descriptor_set)),
          m_render_pass(std::move(rhs.m_render_pass)),
          m_framebuffers(std::move(rhs.m_framebuffers)),
          m_vertex_buffer_data(std::move(rhs.m_vertex_buffer_data)),
          m_graphics_pipeline(std::move(rhs.m_graphics_pipeline)),
          m_image_acquired_semaphores(std::move(rhs.m_image_acquired_semaphores)),
          m_render_finished_semaphores(std::move(rhs.m_render_finished_semaphores)),
          m_in_flight_fences(std::move(rhs.m_in_flight_fences))
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        ,
          m_debug_utils_messenger(std::move(rhs.m_debug_utils_messenger))
#endif
              {};

    /**
     * @brief Begin command buffer and render pass. Set viewport and scissor.
     */
    bool VulkanRenderer::StartRenderpass(uint32_t& image_index)
    {
        vk::Result result;
        std::tie(result, image_index) = m_swapchain_data.swap_chain.acquireNextImage(
            vk::Meow::k_fence_timeout, **m_image_acquired_semaphores[m_current_frame_index]);
        assert(result == vk::Result::eSuccess);
        assert(image_index < m_swapchain_data.images.size());

        m_command_buffers[m_current_frame_index].reset();
        m_command_buffers[m_current_frame_index].begin({});

        m_command_buffers[m_current_frame_index].setViewport(
            0,
            vk::Viewport(0.0f,
                         0.0f,
                         static_cast<float>(m_surface_data.extent.width),
                         static_cast<float>(m_surface_data.extent.height),
                         0.0f,
                         1.0f));
        m_command_buffers[m_current_frame_index].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_surface_data.extent));

        std::array<vk::ClearValue, 2> clear_values;
        clear_values[0].color        = vk::ClearColorValue(0.2f, 0.2f, 0.2f, 0.2f);
        clear_values[1].depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
        vk::RenderPassBeginInfo render_pass_begin_info(*m_render_pass,
                                                       *m_framebuffers[image_index],
                                                       vk::Rect2D(vk::Offset2D(0, 0), m_surface_data.extent),
                                                       clear_values);
        m_command_buffers[m_current_frame_index].beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

        return true;
    }

    /**
     * @brief End render pass and command buffer. Submit graphics queue. Present.
     */
    void VulkanRenderer::EndRenderpass(uint32_t& image_index)
    {
        m_command_buffers[m_current_frame_index].endRenderPass();
        m_command_buffers[m_current_frame_index].end();

        vk::PipelineStageFlags wait_destination_stage_mask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        vk::SubmitInfo         submit_info(**m_image_acquired_semaphores[m_current_frame_index],
                                   wait_destination_stage_mask,
                                   *m_command_buffers[m_current_frame_index],
                                   **m_render_finished_semaphores[m_current_frame_index]);
        m_graphics_queue.submit(submit_info, **m_in_flight_fences[m_current_frame_index]);

        while (vk::Result::eTimeout == m_logical_device.waitForFences({**m_in_flight_fences[m_current_frame_index]},
                                                                      VK_TRUE,
                                                                      vk::Meow::k_fence_timeout))
            ;
        m_logical_device.resetFences({**m_in_flight_fences[m_current_frame_index]});

        vk::PresentInfoKHR present_info(
            **m_render_finished_semaphores[m_current_frame_index], *m_swapchain_data.swap_chain, image_index);
        vk::Result result = m_present_queue.presentKHR(present_info);
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

        m_current_frame_index = (m_current_frame_index + 1) % vk::Meow::k_max_frames_in_flight;
    }

    void VulkanRenderer::Update(float frame_time)
    {
        // TODO: temp
        if (g_runtime_global_context.m_input_system->GetButton("Left")->GetAction() == InputAction::Press)
        {
            RUNTIME_INFO("Press Button Left!");

            m_camera_position += glm::vec3(-1.0f, 0.0f, 0.0f) * frame_time;

            // Update Uniform Buffer
            // TODO: Create a Camera
            UBOData    ubo_data;
            glm::ivec2 window_size = g_runtime_global_context.m_window->GetSize();

            BoundingBox bounding     = m_model->GetBounding();
            glm::vec3   bound_size   = bounding.max - bounding.min;
            glm::vec3   bound_center = bounding.min + bound_size * 0.5f;

            glm::mat4 model_matrix(1.0f);
            model_matrix = glm::translate(model_matrix, m_camera_position);
            // model_matrix = glm::scale(model_matrix, Scale);
            // model_matrix = glm::rotate(model_matrix, rotAngle, Rotation);
            ubo_data.model      = model_matrix;
            ubo_data.projection = glm::perspectiveFovRH_ZO(
                glm::pi<float>() / 4.0f, (float)window_size[0], (float)window_size[1], 0.1f, 1000.0f);
            vk::Meow::CopyToDevice(m_uniform_buffer_data.device_memory, ubo_data);
        }

        uint32_t image_index;
        StartRenderpass(image_index);

        m_command_buffers[m_current_frame_index].bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline);
        m_command_buffers[m_current_frame_index].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, {*m_descriptor_set}, nullptr);
        m_model->Draw(m_command_buffers[m_current_frame_index]);

        EndRenderpass(image_index);
    }
} // namespace Meow