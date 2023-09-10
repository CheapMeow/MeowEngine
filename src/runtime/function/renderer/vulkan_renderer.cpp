#include "vulkan_renderer.h"
#include "core/log/log.h"
#include "function/renderer/utils/vulkan_math_utils.hpp"
#include "function/renderer/utils/vulkan_shader_utils.hpp"

#include "SPIRV/GlslangToSpv.h"
#include <map>

namespace Meow
{
    /**
     * @brief Create Vulkan context.
     */
    void VulkanRenderer::CreateContext()
    {
        m_vulkan_context = std::make_shared<vk::raii::Context>();

#ifdef MEOW_DEBUG
        vk::Meow::LogVulkanAPIVersion(m_vulkan_context->enumerateInstanceVersion());
#endif
    }

    /**
     * @brief Create the Vulkan instance.
     * <p> If build in Debug mode, create DebugUtilsMessengerEXT at the same time.
     *
     * @param required_instance_extensions The required Vulkan instance extensions.
     * @param required_validation_layers The required Vulkan validation layers
     */
    void VulkanRenderer::CreateInstance(std::vector<const char*> const& required_instance_extensions_base,
                                        std::vector<const char*> const& required_validation_layers_base)
    {
        // prepare for create vk::InstanceCreateInfo

        std::vector<vk::ExtensionProperties> available_instance_extensions =
            m_vulkan_context->enumerateInstanceExtensionProperties();

        std::vector<const char*> required_instance_extensions =
            vk::Meow::GetRequiredInstanceExtensions(required_instance_extensions_base);

        if (!vk::Meow::ValidateExtensions(required_instance_extensions, available_instance_extensions))
        {
            throw std::runtime_error("Required instance extensions are missing.");
        }

        std::vector<vk::LayerProperties> supported_validation_layers =
            m_vulkan_context->enumerateInstanceLayerProperties();

        std::vector<const char*> required_validation_layers(required_validation_layers_base);

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

        uint32_t api_version = m_vulkan_context->enumerateInstanceVersion();

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

        m_vulkan_instance = std::make_shared<vk::raii::Instance>(*m_vulkan_context, instance_info);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        m_debug_utils_messenger =
            std::make_shared<vk::raii::DebugUtilsMessengerEXT>(*m_vulkan_instance, debug_utils_create_info);
#endif

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        // we need some additional initializing for this platform!
        if (volkInitialize())
        {
            throw std::runtime_error("Failed to initialize volk.");
        }
        volkLoadInstance(instance);
#endif
    }

    /**
     * @brief Rank all physical devices and choose the one with highest score.
     */
    void VulkanRenderer::CreatePhysicalDevice()
    {
        vk::raii::PhysicalDevices gpus(*m_vulkan_instance);

        // Maps to hold devices and sort by rank.
        std::multimap<uint32_t, vk::raii::PhysicalDevice> ranked_devices;
        auto                                              where = ranked_devices.end();

        // Iterates through all devices and rate their suitability.
        for (const auto& gpu : gpus)
            where =
                ranked_devices.insert(where, {vk::Meow::ScorePhysicalDevice(gpu, k_required_device_extensions), gpu});

        // Checks to make sure the best candidate scored higher than 0  rbegin points to last element of ranked
        // devices(highest rated), first is its rating.
        if (ranked_devices.rbegin()->first > 0)
            m_gpu = std::make_shared<vk::raii::PhysicalDevice>(ranked_devices.rbegin()->second);
    }

    /**
     * @brief Create surface data and create surface in surface data ctor.
     */
    void VulkanRenderer::CreateSurface()
    {
        auto         size = m_window.lock()->GetSize();
        vk::Extent2D extent(size.x, size.y);

        m_surface_data =
            std::make_shared<vk::Meow::SurfaceData>(*m_vulkan_instance, m_window.lock()->GetGLFWWindow(), extent);
    }

    /**
     * @brief Create the Vulkan physical device and logical device.
     *
     * @param context A Vulkan context with an instance already set up.
     * @param required_device_extensions The required Vulkan device extensions.
     */
    void VulkanRenderer::CreateLogicalDeviceAndQueue()
    {
        std::vector<vk::ExtensionProperties> device_extensions = m_gpu->enumerateDeviceExtensionProperties();

        if (!vk::Meow::ValidateExtensions(k_required_device_extensions, device_extensions))
        {
            throw std::runtime_error("Required device extensions are missing, will try without.");
        }

        auto indexs = vk::Meow::FindGraphicsAndPresentQueueFamilyIndex(*m_gpu, (*m_surface_data).surface);
        m_graphics_queue_family_index = indexs.first;
        m_present_queue_family_index  = indexs.second;

        // Create a device with one queue
        float                     queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_info({}, m_graphics_queue_family_index, 1, &queue_priority);
        vk::DeviceCreateInfo      device_info({}, queue_info, {}, k_required_device_extensions);

        m_logical_device = std::make_shared<vk::raii::Device>(*m_gpu, device_info);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        volkLoadDevice(**m_logical_device);
#endif

        m_graphics_queue = std::make_shared<vk::raii::Queue>(*m_logical_device, m_graphics_queue_family_index, 0);
        m_present_queue  = std::make_shared<vk::raii::Queue>(*m_logical_device, m_present_queue_family_index, 0);
    }

    /**
     * @brief Create command pool and command buffer.
     * @todo Support multi thread.
     */
    void VulkanRenderer::CreateCommandBuffer()
    {
        vk::CommandPoolCreateInfo command_pool_create_info({}, m_graphics_queue_family_index);
        m_command_pool = std::make_shared<vk::raii::CommandPool>(*m_logical_device, command_pool_create_info);

        vk::CommandBufferAllocateInfo command_buffer_allocate_info(
            **m_command_pool, vk::CommandBufferLevel::ePrimary, 1);
        m_command_buffer = std::make_shared<vk::raii::CommandBuffer>(
            std::move(vk::raii::CommandBuffers(*m_logical_device, command_buffer_allocate_info).front()));
    }

    /**
     * @brief Create swapchain data and create swapchain in swapchain data ctor.
     */
    void VulkanRenderer::CreateSwapChain()
    {
        m_swapchain_data = std::make_shared<vk::Meow::SwapChainData>(*m_gpu,
                                                                     *m_logical_device,
                                                                     (*m_surface_data).surface,
                                                                     (*m_surface_data).extent,
                                                                     vk::ImageUsageFlagBits::eColorAttachment |
                                                                         vk::ImageUsageFlagBits::eTransferSrc,
                                                                     nullptr,
                                                                     m_graphics_queue_family_index,
                                                                     m_present_queue_family_index);
    }

    /**
     * @brief Create depth buffer data and create depth buffer in depth buffer data ctor.
     */
    void VulkanRenderer::CreateDepthBuffer()
    {
        m_depth_buffer_data = std::make_shared<vk::Meow::DepthBufferData>(
            *m_gpu, *m_logical_device, vk::Format::eD16Unorm, (*m_surface_data).extent);
    }

    /**
     * @brief Create uniform buffer data and create uniform buffer in uniform buffer data ctor.
     */
    void VulkanRenderer::CreateUniformBuffer()
    {
        m_uniform_buffer_data = std::make_shared<vk::Meow::BufferData>(
            *m_gpu, *m_logical_device, sizeof(glm::mat4x4), vk::BufferUsageFlagBits::eUniformBuffer);
        glm::mat4x4 mvpc_matrix = vk::Meow::CreateModelViewProjectionClipMatrix((*m_surface_data).extent);
        vk::Meow::CopyToDevice((*m_uniform_buffer_data).device_memory, mvpc_matrix);
    }

    /**
     * @brief Create descriptor set layout and pipeline layout
     */
    void VulkanRenderer::CreatePipelineLayout()
    {
        m_descriptor_set_layout = std::make_shared<vk::raii::DescriptorSetLayout>(vk::Meow::MakeDescriptorSetLayout(
            *m_logical_device, {{vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}}));

        vk::PipelineLayoutCreateInfo pipeline_layout_create_info({}, **m_descriptor_set_layout);
        m_pipeline_layout = std::make_shared<vk::raii::PipelineLayout>(*m_logical_device, pipeline_layout_create_info);
    }

    /**
     * @brief Create descriptor pool and descriptor set.
     */
    void VulkanRenderer::CreateDescriptorSet()
    {
        // create a descriptor pool
        vk::DescriptorPoolSize       pool_size(vk::DescriptorType::eUniformBuffer, 1);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, pool_size);
        m_descriptor_pool = std::make_shared<vk::raii::DescriptorPool>(*m_logical_device, descriptor_pool_create_info);

        // allocate a descriptor set
        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(**m_descriptor_pool, **m_descriptor_set_layout);
        m_descriptor_set = std::make_shared<vk::raii::DescriptorSet>(
            std::move(vk::raii::DescriptorSets(*m_logical_device, descriptor_set_allocate_info).front()));

        vk::Meow::UpdateDescriptorSets(
            *m_logical_device,
            *m_descriptor_set,
            {{vk::DescriptorType::eUniformBuffer, (*m_uniform_buffer_data).buffer, VK_WHOLE_SIZE, nullptr}},
            {});
    }

    /**
     * @brief Create render pass.
     */
    void VulkanRenderer::CreateRenderPass()
    {
        vk::Format color_format =
            vk::Meow::PickSurfaceFormat((*m_gpu).getSurfaceFormatsKHR(*(*m_surface_data).surface)).format;
        m_render_pass = std::make_shared<vk::raii::RenderPass>(
            std::move(vk::Meow::MakeRenderPass(*m_logical_device, color_format, (*m_depth_buffer_data).format)));
    }

    /**
     * @brief Create shaders using glslang, from glsl to spv.
     */
    void VulkanRenderer::CreateShaders()
    {
        glslang::InitializeProcess();
        m_vertex_shader_module   = std::make_shared<vk::raii::ShaderModule>(std::move(
            vk::Meow::MakeShaderModule(*m_logical_device, vk::ShaderStageFlagBits::eVertex, vertexShaderText_PC_C)));
        m_fragment_shader_module = std::make_shared<vk::raii::ShaderModule>(std::move(
            vk::Meow::MakeShaderModule(*m_logical_device, vk::ShaderStageFlagBits::eFragment, fragmentShaderText_C_C)));
        glslang::FinalizeProcess();
    }

    /**
     * @brief Create framebuffer.
     */
    void VulkanRenderer::CreateFrameBuffer()
    {
        m_framebuffers = std::make_shared<std::vector<vk::raii::Framebuffer>>(
            std::move(vk::Meow::MakeFramebuffers(*m_logical_device,
                                                 *m_render_pass,
                                                 (*m_swapchain_data).image_views,
                                                 &(*m_depth_buffer_data).image_view,
                                                 (*m_surface_data).extent)));
    }

    VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) : m_window(window)
    {
        CreateContext();
        CreateInstance({VK_KHR_SURFACE_EXTENSION_NAME}, {});
        CreatePhysicalDevice();
        CreateSurface();
        CreateLogicalDeviceAndQueue();
        CreateCommandBuffer();
        CreateSwapChain();
        CreateDepthBuffer();
        CreateUniformBuffer();
        CreatePipelineLayout();
        CreateDescriptorSet();
        CreateRenderPass();
        CreateShaders();
        CreateFrameBuffer();
    }

    VulkanRenderer::~VulkanRenderer() {}
} // namespace Meow