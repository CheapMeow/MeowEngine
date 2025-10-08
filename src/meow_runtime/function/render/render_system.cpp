#include "render_system.h"

#include "pch.h"

#include "function/global/runtime_context.h"
#include "function/render/buffer_data/surface_data.h"

#include <map>
#include <volk.h>
namespace Meow
{
    std::vector<const char*> k_required_device_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
    };

    RenderSystem::RenderSystem()
    {
        CreateVulkanInstance();
        CreatePhysicalDevice();
        CreateLogicalDevice();
        CreateCommandPool();
        CreateDescriptorAllocator();
    }

    RenderSystem::~RenderSystem()
    {
        m_descriptor_allocator        = nullptr;
        m_command_pool                = nullptr;
        m_onetime_submit_command_pool = nullptr;
        m_present_queue               = nullptr;
        m_graphics_queue              = nullptr;
        m_logical_device              = nullptr;
        m_physical_device             = nullptr;
        m_vulkan_instance             = nullptr;
    }

    void RenderSystem::Shutdown() { m_logical_device.waitIdle(); }

    void RenderSystem::CreateVulkanInstance()
    {
#ifdef MEOW_DEBUG
        LogVulkanAPIVersion(m_vulkan_context.enumerateInstanceVersion());
#endif

        // prepare for create vk::InstanceCreateInfo
        std::vector<vk::ExtensionProperties> available_instance_extensions =
            m_vulkan_context.enumerateInstanceExtensionProperties();
        std::vector<const char*> required_instance_extensions = GetRequiredInstanceExtensions({
            VK_KHR_SURFACE_EXTENSION_NAME,
        });
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

        bool m_is_validation_layer_found = false;
        m_is_validation_layer_found      = ValidateLayers(required_validation_layers, supported_validation_layers);
        if (m_is_validation_layer_found)
        {
            MEOW_INFO("Enabled Validation Layers:");
            for (const auto& layer : required_validation_layers)
            {
                MEOW_INFO("	\t{}", layer);
            }
        }
        else
        {
            MEOW_ERROR("Required validation layers are missing.");
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

    void RenderSystem::CreatePhysicalDevice()
    {
        vk::raii::PhysicalDevices physical_devices(m_vulkan_instance);

        // Maps to hold devices and sort by rank.
        std::multimap<uint32_t, vk::raii::PhysicalDevice> ranked_devices;
        auto                                              where = ranked_devices.end();
        // Iterates through all devices and rate their suitability.
        for (const auto& physical_device : physical_devices)
            where = ranked_devices.insert(
                where, {ScorePhysicalDevice(physical_device, k_required_device_extensions), physical_device});
        // Checks to make sure the best candidate scored higher than 0  rbegin points to last element of ranked
        // devices(highest rated), first is its rating.
        if (ranked_devices.rbegin()->first < 0)
        {
            throw std::runtime_error("Best physical_device get negitive score.");
        }

        m_physical_device = vk::raii::PhysicalDevice(ranked_devices.rbegin()->second);

        m_msaa_samples = GetMaxUsableSampleCount(m_physical_device);

        std::vector<vk::ExtensionProperties> device_extensions = m_physical_device.enumerateDeviceExtensionProperties();
        std::vector<const char*>             depth_extension   = {VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME};

        m_depth_writeback_resolve_supported = ValidateExtensions(depth_extension, device_extensions);
        if (m_depth_writeback_resolve_supported)
        {
            m_supported_depth_resolve_mode_list = PrepareDepthResolveModeList(m_vulkan_context, m_physical_device);

            if (m_supported_depth_resolve_mode_list.size() > 0 &&
                m_supported_depth_resolve_mode_list[0] != vk::ResolveModeFlagBits::eNone)
            {
                m_current_depth_resolve_mode = m_supported_depth_resolve_mode_list[0];
            }
        }
        else
        {
            m_supported_depth_resolve_mode_list = {vk::ResolveModeFlagBits::eNone};
            m_current_depth_resolve_mode        = vk::ResolveModeFlagBits::eNone;
        }
    }

    void RenderSystem::CreateLogicalDevice()
    {
        // Logical Device

        std::vector<vk::ExtensionProperties> device_extensions = m_physical_device.enumerateDeviceExtensionProperties();
        if (!ValidateExtensions(k_required_device_extensions, device_extensions))
        {
            throw std::runtime_error("Required device extensions are missing, will try without.");
        }

        // temp SurfaceData
        vk::Extent2D extent(1080, 720);
        SurfaceData  surface_data(
            m_vulkan_instance, g_runtime_context.window_system->GetCurrentFocusGLFWWindow(), extent);

        auto indexs                   = FindGraphicsAndPresentQueueFamilyIndex(m_physical_device, surface_data.surface);
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
        m_logical_device = vk::raii::Device(m_physical_device, device_info);

#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        volkLoadDevice(*logical_device);
#endif

        m_compute_queue_family_index = FindComputeQueueFamilyIndex(m_physical_device);

        m_graphics_queue = vk::raii::Queue(m_logical_device, m_graphics_queue_family_index, 0);
        m_present_queue  = vk::raii::Queue(m_logical_device, m_present_queue_family_index, 0);
        m_compute_queue  = vk::raii::Queue(m_logical_device, m_compute_queue_family_index, 0);
    }

    void RenderSystem::CreateCommandPool()
    {
        {
            vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eTransient,
                                                               m_graphics_queue_family_index);
            m_onetime_submit_command_pool = vk::raii::CommandPool(m_logical_device, command_pool_create_info);
        }

        {
            vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
                                                               m_graphics_queue_family_index);
            m_command_pool = vk::raii::CommandPool(m_logical_device, command_pool_create_info);
        }
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
} // namespace Meow
