#include "vulkan_renderer.h"
#include "core/log/log.h"
#include "function/renderer/utils/geometries.hpp"
#include "function/renderer/utils/vulkan_math_utils.hpp"
#include "function/renderer/utils/vulkan_shader_utils.hpp"

#include "SPIRV/GlslangToSpv.h"
#include <map>

namespace Meow
{
    VulkanRenderer::VulkanRenderer(std::shared_ptr<Window>               window,
                                   vk::raii::Context&                    vulkan_context,
                                   vk::raii::Instance&                   vulkan_instance,
                                   vk::raii::PhysicalDevice&             gpu,
                                   vk::Meow::SurfaceData&                surface_data,
                                   uint32_t                              graphics_queue_family_index,
                                   uint32_t                              present_queue_family_index,
                                   vk::raii::Device&                     logical_device,
                                   vk::raii::Queue&                      graphics_queue,
                                   vk::raii::Queue&                      present_queue,
                                   vk::raii::CommandPool&                command_pool,
                                   std::vector<vk::raii::CommandBuffer>& command_buffers,
                                   vk::Meow::SwapChainData&              swapchain_data,
                                   vk::Meow::DepthBufferData&            depth_buffer_data,
                                   vk::Meow::BufferData&                 uniform_buffer_data,
                                   vk::raii::DescriptorSetLayout&        descriptor_set_layout,
                                   vk::raii::PipelineLayout&             pipeline_layout,
                                   vk::raii::DescriptorPool&             descriptor_pool,
                                   vk::raii::DescriptorSet&              descriptor_set,
                                   vk::raii::RenderPass&                 render_pass,
                                   vk::raii::ShaderModule&               vertex_shader_module,
                                   vk::raii::ShaderModule&               fragment_shader_module,
                                   std::vector<vk::raii::Framebuffer>&   framebuffers,
                                   vk::Meow::BufferData&                 vertex_buffer_data,
                                   vk::raii::Pipeline&                   graphics_pipeline
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                                   ,
                                   vk::raii::DebugUtilsMessengerEXT& debug_utils_messenger
#endif
                                   ) :
        m_window(window),
        m_vulkan_context(std::move(vulkan_context)), m_vulkan_instance(std::move(vulkan_instance)),
        m_gpu(std::move(gpu)), m_surface_data(std::move(surface_data)),
        m_graphics_queue_family_index(graphics_queue_family_index),
        m_present_queue_family_index(present_queue_family_index), m_logical_device(std::move(logical_device)),
        m_graphics_queue(std::move(graphics_queue)), m_present_queue(std::move(present_queue)),
        m_command_pool(std::move(command_pool)), m_command_buffers(std::move(command_buffers)),
        m_swapchain_data(std::move(swapchain_data)), m_depth_buffer_data(std::move(depth_buffer_data)),
        m_uniform_buffer_data(std::move(uniform_buffer_data)),
        m_descriptor_set_layout(std::move(descriptor_set_layout)), m_pipeline_layout(std::move(pipeline_layout)),
        m_descriptor_pool(std::move(descriptor_pool)), m_descriptor_set(std::move(descriptor_set)),
        m_render_pass(std::move(render_pass)), m_vertex_shader_module(std::move(vertex_shader_module)),
        m_fragment_shader_module(std::move(fragment_shader_module)), m_framebuffers(std::move(framebuffers)),
        m_vertex_buffer_data(std::move(vertex_buffer_data)), m_graphics_pipeline(std::move(graphics_pipeline))
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        ,
        m_debug_utils_messenger(std::move(debug_utils_messenger))
#endif
    {}

    /**
     * @brief In order to support move ctor.
     */
    VulkanRenderer::VulkanRenderer(VulkanRenderer&& rhs) VULKAN_HPP_NOEXCEPT
        : m_window(rhs.m_window),
          m_vulkan_context(std::move(rhs.m_vulkan_context)),
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
          m_vertex_shader_module(std::move(rhs.m_vertex_shader_module)),
          m_fragment_shader_module(std::move(rhs.m_fragment_shader_module)),
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

    VulkanRenderer::~VulkanRenderer() {}

    /**
     * @brief Factory mode of creating instance of VulkanRenderer.
     *
     * Use factory mode because VulkanRenderer has many members that don't have default ctor,
     * so VulkanRenderer itself can't call default ctor, which will call all members' default ctor.
     *
     * One solution is using pointer to these class members, other is initializer list in ctor.
     *
     * To use initializer list in ctor, a custom ctor receiving initial members' values should be defined.
     * And where to call the custom ctor? You can call it as you like, but put it into VulkanRenderer class seems more
     * clear.
     *
     * After that, move ctor should be defined to support usage like: p =
     * make_unique<VulkanRenderer>(CreateVulkanRenderer(...));
     */
    VulkanRenderer VulkanRenderer::CreateRenderer(std::shared_ptr<Window> window)
    {
        // --------------Create Context--------------

        vk::raii::Context vulkan_context;

#ifdef MEOW_DEBUG
        vk::Meow::LogVulkanAPIVersion(vulkan_context.enumerateInstanceVersion());
#endif

        // --------------Create Instance--------------

        // prepare for create vk::InstanceCreateInfo

        std::vector<vk::ExtensionProperties> available_instance_extensions =
            vulkan_context.enumerateInstanceExtensionProperties();

        std::vector<const char*> required_instance_extensions =
            vk::Meow::GetRequiredInstanceExtensions({VK_KHR_SURFACE_EXTENSION_NAME});

        if (!vk::Meow::ValidateExtensions(required_instance_extensions, available_instance_extensions))
        {
            throw std::runtime_error("Required instance extensions are missing.");
        }

        std::vector<vk::LayerProperties> supported_validation_layers =
            vulkan_context.enumerateInstanceLayerProperties();

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

        uint32_t api_version = vulkan_context.enumerateInstanceVersion();

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

        vk::raii::Instance vulkan_instance(vulkan_context, instance_info);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        vk::raii::DebugUtilsMessengerEXT debug_utils_messenger(vulkan_instance, debug_utils_create_info);
#endif

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        // we need some additional initializing for this platform!
        if (volkInitialize())
        {
            throw std::runtime_error("Failed to initialize volk.");
        }
        volkLoadInstance(instance);
#endif

        // --------------Create Physical Device--------------

        vk::raii::PhysicalDevices gpus(vulkan_instance);

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

        // --------------Create Surface--------------

        auto         size = window->GetSize();
        vk::Extent2D extent(size.x, size.y);

        vk::Meow::SurfaceData surface_data(vulkan_instance, window->GetGLFWWindow(), extent);

        // --------------Create Logical Device--------------

        std::vector<vk::ExtensionProperties> device_extensions = gpu.enumerateDeviceExtensionProperties();

        if (!vk::Meow::ValidateExtensions(vk::Meow::k_required_device_extensions, device_extensions))
        {
            throw std::runtime_error("Required device extensions are missing, will try without.");
        }

        auto     indexs = vk::Meow::FindGraphicsAndPresentQueueFamilyIndex(gpu, surface_data.surface);
        uint32_t graphics_queue_family_index = indexs.first;
        uint32_t present_queue_family_index  = indexs.second;

        // Create a device with one queue
        float                     queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_info({}, graphics_queue_family_index, 1, &queue_priority);
        vk::DeviceCreateInfo      device_info({}, queue_info, {}, vk::Meow::k_required_device_extensions);

        vk::raii::Device logical_device(gpu, device_info);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        volkLoadDevice(*logical_device);
#endif

        // --------------Create Queue--------------

        vk::raii::Queue graphics_queue(logical_device, graphics_queue_family_index, 0);
        vk::raii::Queue present_queue(logical_device, present_queue_family_index, 0);

        // --------------Create Command Buffer--------------

        vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eTransient |
                                                               vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                           graphics_queue_family_index);
        vk::raii::CommandPool     command_pool(logical_device, command_pool_create_info);

        vk::CommandBufferAllocateInfo command_buffer_allocate_info(
            *command_pool, vk::CommandBufferLevel::ePrimary, vk::Meow::k_max_frames_in_flight);
        std::vector<vk::raii::CommandBuffer> command_buffers(
            std::move(vk::raii::CommandBuffers(logical_device, command_buffer_allocate_info)));

        // --------------Create SwapChain--------------

        vk::Meow::SwapChainData swapchain_data(gpu,
                                               logical_device,
                                               surface_data.surface,
                                               surface_data.extent,
                                               vk::ImageUsageFlagBits::eColorAttachment |
                                                   vk::ImageUsageFlagBits::eTransferSrc,
                                               nullptr,
                                               graphics_queue_family_index,
                                               present_queue_family_index);

        // --------------Create Depth Buffer--------------

        vk::Meow::DepthBufferData depth_buffer_data(gpu, logical_device, vk::Format::eD16Unorm, surface_data.extent);

        // --------------Create Uniform Buffer--------------

        vk::Meow::BufferData uniform_buffer_data(
            gpu, logical_device, sizeof(glm::mat4x4), vk::BufferUsageFlagBits::eUniformBuffer);
        glm::mat4x4 mvpc_matrix = vk::Meow::CreateModelViewProjectionClipMatrix(surface_data.extent);
        vk::Meow::CopyToDevice(uniform_buffer_data.device_memory, mvpc_matrix);

        // --------------Create Pipeline Layout--------------

        vk::raii::DescriptorSetLayout descriptor_set_layout(vk::Meow::MakeDescriptorSetLayout(
            logical_device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}}));

        vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, *descriptor_set_layout);
        vk::raii::PipelineLayout     pipeline_layout(logical_device, pipeline_layout_create_info);

        // --------------Create Descriptor Set--------------

        // create a descriptor pool
        vk::DescriptorPoolSize       pool_size(vk::DescriptorType::eUniformBuffer, 1);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, pool_size);
        vk::raii::DescriptorPool descriptor_pool(logical_device, descriptor_pool_create_info);

        // allocate a descriptor set
        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(*descriptor_pool, *descriptor_set_layout);
        vk::raii::DescriptorSet       descriptor_set(
            std::move(vk::raii::DescriptorSets(logical_device, descriptor_set_allocate_info).front()));

        vk::Meow::UpdateDescriptorSets(
            logical_device,
            descriptor_set,
            {{vk::DescriptorType::eUniformBuffer, uniform_buffer_data.buffer, VK_WHOLE_SIZE, nullptr}},
            {});

        // --------------Create Render Pass--------------

        vk::Format color_format = vk::Meow::PickSurfaceFormat((gpu).getSurfaceFormatsKHR(*surface_data.surface)).format;
        vk::raii::RenderPass render_pass(
            std::move(vk::Meow::MakeRenderPass(logical_device, color_format, depth_buffer_data.format)));

        // --------------Create Shaders--------------

        glslang::InitializeProcess();
        vk::raii::ShaderModule vertex_shader_module(std::move(
            vk::Meow::MakeShaderModule(logical_device, vk::ShaderStageFlagBits::eVertex, vertexShaderText_PC_C)));
        vk::raii::ShaderModule fragment_shader_module(std::move(
            vk::Meow::MakeShaderModule(logical_device, vk::ShaderStageFlagBits::eFragment, fragmentShaderText_C_C)));
        glslang::FinalizeProcess();

        // --------------Create Frame Buffers--------------

        std::vector<vk::raii::Framebuffer> framebuffers(
            std::move(vk::Meow::MakeFramebuffers(logical_device,
                                                 render_pass,
                                                 swapchain_data.image_views,
                                                 &depth_buffer_data.image_view,
                                                 surface_data.extent)));

        // --------------Create Vertex Buffer--------------

        // TODO: Remove temp vertex data
        vk::Meow::BufferData vertex_buffer_data(
            gpu, logical_device, sizeof(ColoredCubeData), vk::BufferUsageFlagBits::eVertexBuffer);
        vk::Meow::CopyToDevice(
            vertex_buffer_data.device_memory, ColoredCubeData, sizeof(ColoredCubeData) / sizeof(ColoredCubeData[0]));

        // --------------Create Pipeline--------------

        vk::raii::PipelineCache pipeline_cache(logical_device, vk::PipelineCacheCreateInfo());
        vk::raii::Pipeline      graphics_pipeline(std::move(vk::Meow::MakeGraphicsPipeline(
            logical_device,
            pipeline_cache,
            vertex_shader_module,
            nullptr,
            fragment_shader_module,
            nullptr,
            vk::Meow::checked_cast<uint32_t>(sizeof(ColoredCubeData[0])),
            {{vk::Format::eR32G32B32A32Sfloat, 0}, {vk::Format::eR32G32B32A32Sfloat, 16}},
            vk::FrontFace::eClockwise,
            true,
            pipeline_layout,
            render_pass)));

        // --------------Return VulkanRenderer--------------

        return
        {
            window, vulkan_context, vulkan_instance, gpu, surface_data, graphics_queue_family_index,
                present_queue_family_index, logical_device, graphics_queue, present_queue, command_pool,
                command_buffers, swapchain_data, depth_buffer_data, uniform_buffer_data, descriptor_set_layout,
                pipeline_layout, descriptor_pool, descriptor_set, render_pass, vertex_shader_module,
                fragment_shader_module, framebuffers, vertex_buffer_data, graphics_pipeline
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                ,
                debug_utils_messenger
#endif
        };
    }

    /**
     * @brief Initialize sync object.
     * Because they have no default ctor, so they can't be T of vector<T>, but vector<pointer<T>> is allowed.
     */
    void VulkanRenderer::Init()
    {
        // --------------Create Sync Objects--------------

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

    void VulkanRenderer::Update()
    {
        uint32_t image_index;
        StartRenderpass(image_index);

        m_command_buffers[m_current_frame_index].bindPipeline(vk::PipelineBindPoint::eGraphics, *m_graphics_pipeline);
        m_command_buffers[m_current_frame_index].bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, *m_pipeline_layout, 0, {*m_descriptor_set}, nullptr);
        m_command_buffers[m_current_frame_index].bindVertexBuffers(0, {*m_vertex_buffer_data.buffer}, {0});
        m_command_buffers[m_current_frame_index].draw(12 * 3, 1, 0, 0);

        EndRenderpass(image_index);
    }
} // namespace Meow