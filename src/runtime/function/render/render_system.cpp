#include "render_system.h"

#include "pch.h"

#include "function/global/runtime_global_context.h"

#include <map>
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

        m_is_validation_layer_found = ValidateLayers(required_validation_layers, supported_validation_layers);
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

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
    void RenderSystem::CreateDebugUtilsMessengerEXT()
    {
        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info = MakeDebugUtilsMessengerCreateInfoEXT();
        m_debug_utils_messenger = vk::raii::DebugUtilsMessengerEXT(m_vulkan_instance, debug_utils_create_info);
    }
#endif

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
    }

    void RenderSystem::CreateLogicalDevice()
    {
        std::vector<vk::ExtensionProperties> device_extensions = m_physical_device.enumerateDeviceExtensionProperties();
        if (!ValidateExtensions(k_required_device_extensions, device_extensions))
        {
            throw std::runtime_error("Required device extensions are missing, will try without.");
        }

        const SurfaceData& surface_data =
            g_runtime_global_context.window_system->GetCurrentFocusWindow()->GetSurfaceData();
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

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        std::string                     object_name = "Logical Device";
        vk::DebugUtilsObjectNameInfoEXT name_info   = {
            vk::ObjectType::eDevice, GetVulkanHandle(*m_logical_device), object_name.c_str(), nullptr};
        m_logical_device.setDebugUtilsObjectNameEXT(name_info);
#endif

        m_graphics_queue = vk::raii::Queue(m_logical_device, m_graphics_queue_family_index, 0);
        m_present_queue  = vk::raii::Queue(m_logical_device, m_present_queue_family_index, 0);
    }

    RenderSystem::RenderSystem() { CreateVulkanInstance(); }

    RenderSystem::~RenderSystem()
    {
        m_logical_device.waitIdle();

        m_onetime_submit_command_pool = nullptr;
        m_present_queue               = nullptr;
        m_graphics_queue              = nullptr;
        m_logical_device              = nullptr;
        m_physical_device             = nullptr;
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        m_debug_utils_messenger = nullptr;
#endif
        m_vulkan_instance = nullptr;
    }

    void RenderSystem::Start()
    {
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        CreateDebugUtilsMessengerEXT();
#endif
        CreatePhysicalDevice();
        CreateLogicalDevice();

        vk::CommandPoolCreateInfo command_pool_create_info(vk::CommandPoolCreateFlagBits::eTransient,
                                                           m_graphics_queue_family_index);
        m_onetime_submit_command_pool = vk::raii::CommandPool(m_logical_device, command_pool_create_info);

        g_runtime_global_context.window_system->GetCurrentFocusWindow()->Init();
    }

    void RenderSystem::Tick(float dt) {}

    std::shared_ptr<ImageData> RenderSystem::CreateTexture(const std::string& file_path)
    {
        FUNCTION_TIMER();

        return ImageData::CreateTexture(
            m_physical_device, m_logical_device, m_onetime_submit_command_pool, m_graphics_queue, file_path);
    }

    std::shared_ptr<Model> RenderSystem::CreateModel(std::vector<float>&&        vertices,
                                                     std::vector<uint32_t>&&     indices,
                                                     BitMask<VertexAttributeBit> attributes)
    {
        FUNCTION_TIMER();

        return std::make_shared<Model>(m_physical_device,
                                       m_logical_device,
                                       m_onetime_submit_command_pool,
                                       m_present_queue,
                                       std::move(vertices),
                                       std::move(indices),
                                       attributes);
    }

    std::shared_ptr<Model> RenderSystem::CreateModel(const std::string&          file_path,
                                                     BitMask<VertexAttributeBit> attributes)
    {
        FUNCTION_TIMER();

        return std::make_shared<Model>(
            m_physical_device, m_logical_device, m_onetime_submit_command_pool, m_present_queue, file_path, attributes);
    }
} // namespace Meow
