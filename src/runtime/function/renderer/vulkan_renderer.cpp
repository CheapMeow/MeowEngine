#include "vulkan_renderer.h"
#include "core/log/log.h"
#include "function/renderer/utils/utils.h"

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

    VulkanRenderer::VulkanRenderer(std::shared_ptr<Window> window) : m_window(window)
    {
        CreateContext();
        CreateInstance({VK_KHR_SURFACE_EXTENSION_NAME}, {});
        CreatePhysicalDevice();
    }

    VulkanRenderer::~VulkanRenderer() {}
} // namespace Meow