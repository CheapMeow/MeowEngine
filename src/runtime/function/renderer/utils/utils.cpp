#include "utils.h"
#include "core/log/log.h"

#include <iomanip>

namespace vk
{
    namespace Meow
    {
        std::vector<const char*>
        GetRequiredInstanceExtensions(std::vector<const char*> const& required_instance_extensions_base)
        {
            std::vector<const char*> required_instance_extensions(required_instance_extensions_base);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            required_instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

#if (defined(VKB_ENABLE_PORTABILITY))
            required_instance_extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
            required_instance_extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

#if defined(VK_USE_PLATFORM_ANDROID_KHR)
            required_instance_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
            required_instance_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_METAL_EXT)
            required_instance_extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
            required_instance_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
            required_instance_extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
            required_instance_extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_DISPLAY_KHR)
            required_instance_extensions.push_back(VK_KHR_DISPLAY_EXTENSION_NAME);
#else
#    pragma error Platform not supported
#endif

            return required_instance_extensions;
        }

        /**
         * @brief Validates a list of required extensions, comparing it with the available ones.
         *
         * @param required A vector containing required extension names.
         * @param available A vk::ExtensionProperties object containing available extensions.
         * @return true if all required extensions are available
         * @return false otherwise
         */
        bool ValidateExtensions(const std::vector<const char*>&             required,
                                const std::vector<vk::ExtensionProperties>& available)
        {
            // inner find_if gives true if the extension was not found
            // outer find_if gives true if none of the extensions were not found, that is if all extensions were found
            return std::find_if(required.begin(), required.end(), [&available](auto extension) {
                       return std::find_if(available.begin(), available.end(), [&extension](auto const& ep) {
                                  return strcmp(ep.extensionName, extension) == 0;
                              }) == available.end();
                   }) == required.end();
        }

        /**
         * @brief Validates a list of required layers, comparing it with the available ones.
         *
         * @param required A vector containing required layer names.
         * @param available A VkLayerProperties object containing available layers.
         * @return true if all required extensions are available
         * @return false otherwise
         */
        bool ValidateLayers(const std::vector<const char*>& required, const std::vector<vk::LayerProperties>& available)
        {
            // inner find_if returns true if the layer was not found
            // outer find_if returns iterator to the not found layer, if any
            auto requiredButNotFoundIt = std::find_if(required.begin(), required.end(), [&available](auto layer) {
                return std::find_if(available.begin(), available.end(), [&layer](auto const& lp) {
                           return strcmp(lp.layerName, layer) == 0;
                       }) == available.end();
            });
            if (requiredButNotFoundIt != required.end())
            {
                RUNTIME_ERROR("Validation Layer {} not found", *requiredButNotFoundIt);
            }
            return (requiredButNotFoundIt == required.end());
        }

        std::vector<const char*>
        GetOptimalValidationLayers(const std::vector<vk::LayerProperties>& supported_instance_layers)
        {
            std::vector<std::vector<const char*>> validation_layer_priority_list = {
                // The preferred validation layer is "VK_LAYER_KHRONOS_validation"
                {"VK_LAYER_KHRONOS_validation"},

                // Otherwise we fallback to using the LunarG meta layer
                {"VK_LAYER_LUNARG_standard_validation"},

                // Otherwise we attempt to enable the individual layers that compose the LunarG meta layer since it
                // doesn't
                // exist
                {
                    "VK_LAYER_GOOGLE_threading",
                    "VK_LAYER_LUNARG_parameter_validation",
                    "VK_LAYER_LUNARG_object_tracker",
                    "VK_LAYER_LUNARG_core_validation",
                    "VK_LAYER_GOOGLE_unique_objects",
                },

                // Otherwise as a last resort we fallback to attempting to enable the LunarG core layer
                {"VK_LAYER_LUNARG_core_validation"}};

            for (auto& validation_layers : validation_layer_priority_list)
            {
                if (ValidateLayers(validation_layers, supported_instance_layers))
                {
                    return validation_layers;
                }

                RUNTIME_ERROR("Couldn't enable validation layers (see log for error) - falling back");
            }

            // Else return nothing
            return {};
        }

        VkBool32 DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                             VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                             VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                             void* /*pUserData*/)
        {
#if !defined(NDEBUG)
            if (static_cast<uint32_t>(pCallbackData->messageIdNumber) == 0x822806fa)
            {
                // Validation Warning: vkCreateInstance(): to enable extension VK_EXT_debug_utils, but this extension is
                // intended to support use by applications when debugging and it is strongly recommended that it be
                // otherwise avoided.
                return VK_FALSE;
            }
            else if (static_cast<uint32_t>(pCallbackData->messageIdNumber) == 0xe8d1a9fe)
            {
                // Validation Performance Warning: Using debug builds of the validation layers *will* adversely affect
                // performance.
                return VK_FALSE;
            }
#endif
            std::stringstream error_str;

            error_str << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ": "
                      << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) << ":\n";
            error_str << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
            error_str << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
            error_str << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
            if (0 < pCallbackData->queueLabelCount)
            {
                error_str << std::string("\t") << "Queue Labels:\n";
                for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++)
                {
                    error_str << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName
                              << ">\n";
                }
            }
            if (0 < pCallbackData->cmdBufLabelCount)
            {
                error_str << std::string("\t") << "CommandBuffer Labels:\n";
                for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++)
                {
                    error_str << std::string("\t\t") << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName
                              << ">\n";
                }
            }
            if (0 < pCallbackData->objectCount)
            {
                error_str << std::string("\t") << "Objects:\n";
                for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
                {
                    error_str << std::string("\t\t") << "Object " << i << "\n";
                    error_str << std::string("\t\t\t") << "objectType   = "
                              << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType))
                              << "\n";
                    error_str << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle
                              << "\n";
                    if (pCallbackData->pObjects[i].pObjectName)
                    {
                        error_str << std::string("\t\t\t") << "objectName   = <"
                                  << pCallbackData->pObjects[i].pObjectName << ">\n";
                    }
                }
            }

            RUNTIME_ERROR(error_str);

            return VK_FALSE;
        }

        vk::DebugUtilsMessengerCreateInfoEXT MakeDebugUtilsMessengerCreateInfoEXT()
        {
            return {
                {},
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
                vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                    vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
                &DebugUtilsMessengerCallback};
        }

        /**
         * @brief Log Vulkan API version from vk::raii::context.
         */
        void LogVulkanAPIVersion(const uint32_t api_version)
        {
            uint32_t major = VK_VERSION_MAJOR(api_version);
            uint32_t minor = VK_VERSION_MINOR(api_version);
            uint32_t patch = VK_VERSION_PATCH(api_version);
            RUNTIME_INFO("Vulkan API Version: {}.{}.{}", major, minor, patch);
        }

        /**
         * @brief Log Vulkan physical device.
         */
        void LogVulkanDevice(const VkPhysicalDeviceProperties&           physical_device_properties,
                             const std::vector<vk::ExtensionProperties>& extension_properties)
        {
            std::stringstream ss;
            switch (static_cast<int32_t>(physical_device_properties.deviceType))
            {
                case 1:
                    ss << "Integrated";
                    break;
                case 2:
                    ss << "Discrete";
                    break;
                case 3:
                    ss << "Virtual";
                    break;
                case 4:
                    ss << "CPU";
                    break;
                default:
                    ss << "Other " << physical_device_properties.deviceType;
            }

            ss << " Physical Device: " << physical_device_properties.deviceID;
            switch (physical_device_properties.vendorID)
            {
                case 0x8086:
                    ss << " \"Intel\"";
                    break;
                case 0x10DE:
                    ss << " \"Nvidia\"";
                    break;
                case 0x1002:
                    ss << " \"AMD\"";
                    break;
                default:
                    ss << " \"" << physical_device_properties.vendorID << '\"';
            }

            ss << " " << std::quoted(physical_device_properties.deviceName) << '\n';

            uint32_t supported_version[3] = {VK_VERSION_MAJOR(physical_device_properties.apiVersion),
                                             VK_VERSION_MINOR(physical_device_properties.apiVersion),
                                             VK_VERSION_PATCH(physical_device_properties.apiVersion)};
            ss << "API Version: " << supported_version[0] << "." << supported_version[1] << "." << supported_version[2]
               << '\n';

            ss << "Extensions: ";
            for (const auto& extension : extension_properties)
                ss << extension.extensionName << ", ";

            ss << "\n\n";
            RUNTIME_INFO(ss.str());
        }

        /**
         * @brief Score physical device according to device type and max image dimesnion
         */
        uint32_t ScorePhysicalDevice(const vk::raii::PhysicalDevice& device,
                                     const std::vector<const char*>& required_device_extensions)
        {
            uint32_t score = 0;

            // Checks if the requested extensions are supported.

            std::vector<vk::ExtensionProperties> extension_properties = device.enumerateDeviceExtensionProperties();

            // Iterates through all extensions requested.
            for (const char* currentExtension : required_device_extensions)
            {
                bool extension_found = false;

                // Checks if the extension is in the available extensions.
                for (const auto& extension : extension_properties)
                {
                    if (strcmp(currentExtension, extension.extensionName) == 0)
                    {
                        extension_found = true;
                        break;
                    }
                }

                // Returns a score of 0 if this device is missing a required extension.
                if (!extension_found)
                    return 0;
            }

            // Obtain the device features and properties of the current device being rateds.
            VkPhysicalDeviceProperties physical_device_properties = device.getProperties();
            VkPhysicalDeviceFeatures   physical_device_features   = device.getFeatures();

#ifdef MEOW_DEBUG
            vk::Meow::LogVulkanDevice(physical_device_properties, extension_properties);
#endif

            // Adds a large score boost for discrete GPUs (dedicated graphics cards).
            if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                score += 1000;

            // Gives a higher score to devices with a higher maximum texture size.
            score += physical_device_properties.limits.maxImageDimension2D;
            return score;
        }

    } // namespace Meow
} // namespace vk