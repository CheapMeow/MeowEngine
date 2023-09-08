#include <vulkan/vulkan_raii.hpp>

#include <iostream>

namespace vk
{
    namespace Meow
    {
        std::vector<const char*>
        GetRequiredInstanceExtensions(std::vector<const char*> const& required_instance_extensions);

        bool ValidateExtensions(const std::vector<const char*>&             required,
                                const std::vector<vk::ExtensionProperties>& available);

        bool ValidateLayers(const std::vector<const char*>&         required,
                            const std::vector<vk::LayerProperties>& available);

        std::vector<const char*>
        GetOptimalValidationLayers(const std::vector<vk::LayerProperties>& supported_instance_layers);

        VKAPI_ATTR VkBool32 VKAPI_CALL
        DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                    VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                    void* /*pUserData*/);

        vk::DebugUtilsMessengerCreateInfoEXT MakeDebugUtilsMessengerCreateInfoEXT();

        void LogVulkanAPIVersion(const vk::raii::Context context);

        void LogVulkanDevice(const VkPhysicalDeviceProperties&         physical_device_properties,
                             const std::vector<VkExtensionProperties>& extension_properties);

        uint32_t ScorePhysicalDevice(const vk::raii::PhysicalDevice& device,
                                     const std::vector<const char*>& required_device_extensions);

    } // namespace Meow
} // namespace vk