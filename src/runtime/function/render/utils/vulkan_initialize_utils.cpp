#include "vulkan_initialize_utils.hpp"
#include "core/log/log.h"

#include <cassert>
#include <iomanip>
#include <limits>
#include <numeric>

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
                          << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType)) << "\n";
                error_str << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle
                          << "\n";
                if (pCallbackData->pObjects[i].pObjectName)
                {
                    error_str << std::string("\t\t\t") << "objectName   = <" << pCallbackData->pObjects[i].pObjectName
                              << ">\n";
                }
            }
        }

        RUNTIME_ERROR("{}", error_str.str());

        return VK_FALSE;
    }

    vk::DebugUtilsMessengerCreateInfoEXT MakeDebugUtilsMessengerCreateInfoEXT()
    {
        return {{},
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
        RUNTIME_INFO("{}", ss.str());
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
        LogVulkanDevice(physical_device_properties, extension_properties);
#endif

        // Adds a large score boost for discrete GPUs (dedicated graphics cards).
        if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 1000;

        // Gives a higher score to devices with a higher maximum texture size.
        score += physical_device_properties.limits.maxImageDimension2D;
        return score;
    }

    uint32_t FindGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queue_family_properties)
    {
        // get the first index into queueFamiliyProperties which supports graphics
        std::vector<vk::QueueFamilyProperties>::const_iterator graphics_queue_family_property = std::find_if(
            queue_family_properties.begin(), queue_family_properties.end(), [](vk::QueueFamilyProperties const& qfp) {
                return qfp.queueFlags & vk::QueueFlagBits::eGraphics;
            });
        assert(graphics_queue_family_property != queue_family_properties.end());
        return static_cast<uint32_t>(std::distance(queue_family_properties.begin(), graphics_queue_family_property));
    }

    std::pair<uint32_t, uint32_t>
    FindGraphicsAndPresentQueueFamilyIndex(vk::raii::PhysicalDevice const& physical_device,
                                           vk::raii::SurfaceKHR const&     surface)
    {
        std::vector<vk::QueueFamilyProperties> queue_family_properties = physical_device.getQueueFamilyProperties();
        assert(queue_family_properties.size() < std::numeric_limits<uint32_t>::max());

        uint32_t graphics_queue_family_index = FindGraphicsQueueFamilyIndex(queue_family_properties);
        if (physical_device.getSurfaceSupportKHR(graphics_queue_family_index, *surface))
        {
            return {graphics_queue_family_index,
                    graphics_queue_family_index}; // the first graphics_queue_family_index does also support presents
        }

        // the graphics_queue_family_index doesn't support present -> look for an other family index that supports
        // both graphics and present
        for (size_t i = 0; i < queue_family_properties.size(); i++)
        {
            if ((queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
            {
                return {static_cast<uint32_t>(i), static_cast<uint32_t>(i)};
            }
        }

        // there's nothing like a single family index that supports both graphics and present -> look for an other
        // family index that supports present
        for (size_t i = 0; i < queue_family_properties.size(); i++)
        {
            if (physical_device.getSurfaceSupportKHR(static_cast<uint32_t>(i), *surface))
            {
                return {graphics_queue_family_index, static_cast<uint32_t>(i)};
            }
        }

        throw std::runtime_error("Could not find queues for both graphics or present -> terminating");
    }

    vk::SurfaceFormatKHR PickSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& formats)
    {
        assert(!formats.empty());
        vk::SurfaceFormatKHR picked_format = formats[0];
        if (formats.size() == 1)
        {
            if (formats[0].format == vk::Format::eUndefined)
            {
                picked_format.format     = vk::Format::eB8G8R8A8Unorm;
                picked_format.colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
            }
        }
        else
        {
            // request several formats, the first found will be used
            vk::Format        requested_formats[]   = {vk::Format::eB8G8R8A8Unorm,
                                                       vk::Format::eR8G8B8A8Unorm,
                                                       vk::Format::eB8G8R8Unorm,
                                                       vk::Format::eR8G8B8Unorm};
            vk::ColorSpaceKHR requested_color_space = vk::ColorSpaceKHR::eSrgbNonlinear;
            for (size_t i = 0; i < sizeof(requested_formats) / sizeof(requested_formats[0]); i++)
            {
                vk::Format requested_format = requested_formats[i];
                auto       it =
                    std::find_if(formats.begin(),
                                 formats.end(),
                                 [requested_format, requested_color_space](vk::SurfaceFormatKHR const& f) {
                                     return (f.format == requested_format) && (f.colorSpace == requested_color_space);
                                 });
                if (it != formats.end())
                {
                    picked_format = *it;
                    break;
                }
            }
        }
        assert(picked_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear);
        return picked_format;
    }

    vk::PresentModeKHR PickPresentMode(std::vector<vk::PresentModeKHR> const& present_modes)
    {
        vk::PresentModeKHR picked_mode = vk::PresentModeKHR::eFifo;
        for (const auto& present_mode : present_modes)
        {
            if (present_mode == vk::PresentModeKHR::eMailbox)
            {
                picked_mode = present_mode;
                break;
            }

            if (present_mode == vk::PresentModeKHR::eImmediate)
            {
                picked_mode = present_mode;
            }
        }
        return picked_mode;
    }

    uint32_t FindMemoryType(vk::PhysicalDeviceMemoryProperties const& memory_properties,
                            uint32_t                                  type_bits,
                            vk::MemoryPropertyFlags                   requirements_mask)
    {
        uint32_t type_index = uint32_t(~0);
        for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++)
        {
            if ((type_bits & 1) &&
                ((memory_properties.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask))
            {
                type_index = i;
                break;
            }
            type_bits >>= 1;
        }
        assert(type_index != uint32_t(~0));
        return type_index;
    }

    vk::raii::DeviceMemory AllocateDeviceMemory(vk::raii::Device const&                   device,
                                                vk::PhysicalDeviceMemoryProperties const& memory_properties,
                                                vk::MemoryRequirements const&             memory_requirements,
                                                vk::MemoryPropertyFlags                   memory_property_flags)
    {
        uint32_t memory_type_index =
            FindMemoryType(memory_properties, memory_requirements.memoryTypeBits, memory_property_flags);
        vk::MemoryAllocateInfo memory_allocate_info(memory_requirements.size, memory_type_index);
        return vk::raii::DeviceMemory(device, memory_allocate_info);
    }

    /**
     * @brief Set the Image Layout.
     *
     * If an image is not initialized in any specific layout, so we need to do a layout transition.
     *
     * Such as you need the driver puts the texture into Linear layout,
     * which is the best for copying data from a buffer into a texture.
     *
     * To perform layout transitions, we need to use pipeline barriers. Pipeline barriers can control how the GPU
     * overlaps commands before and after the barrier, but if you do pipeline barriers with image barriers, the driver
     * can also transform the image to the correct formats and layouts.
     *
     * @param command_buffer Usually use command buffer in upload context
     * @param image Target image need a layout transition
     * @param format Image data format
     * @param old_image_layout Old image layout
     * @param new_image_layout New image layout
     */
    void SetImageLayout(vk::raii::CommandBuffer const& command_buffer,
                        vk::Image                      image,
                        vk::Format                     format,
                        vk::ImageLayout                old_image_layout,
                        vk::ImageLayout                new_image_layout)
    {
        vk::AccessFlags source_access_mask;
        switch (old_image_layout)
        {
            case vk::ImageLayout::eTransferDstOptimal:
                source_access_mask = vk::AccessFlagBits::eTransferWrite;
                break;
            case vk::ImageLayout::ePreinitialized:
                source_access_mask = vk::AccessFlagBits::eHostWrite;
                break;
            case vk::ImageLayout::eGeneral: // source_access_mask is empty
            case vk::ImageLayout::eUndefined:
                break;
            default:
                assert(false);
                break;
        }

        vk::PipelineStageFlags source_stage;
        switch (old_image_layout)
        {
            case vk::ImageLayout::eGeneral:
            case vk::ImageLayout::ePreinitialized:
                source_stage = vk::PipelineStageFlagBits::eHost;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
                source_stage = vk::PipelineStageFlagBits::eTransfer;
                break;
            case vk::ImageLayout::eUndefined:
                source_stage = vk::PipelineStageFlagBits::eTopOfPipe;
                break;
            default:
                assert(false);
                break;
        }

        vk::AccessFlags destination_access_mask;
        switch (new_image_layout)
        {
            case vk::ImageLayout::eColorAttachmentOptimal:
                destination_access_mask = vk::AccessFlagBits::eColorAttachmentWrite;
                break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                destination_access_mask =
                    vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
                break;
            case vk::ImageLayout::eGeneral: // empty destination_access_mask
            case vk::ImageLayout::ePresentSrcKHR:
                break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                destination_access_mask = vk::AccessFlagBits::eShaderRead;
                break;
            case vk::ImageLayout::eTransferSrcOptimal:
                destination_access_mask = vk::AccessFlagBits::eTransferRead;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
                destination_access_mask = vk::AccessFlagBits::eTransferWrite;
                break;
            default:
                assert(false);
                break;
        }

        vk::PipelineStageFlags destination_stage;
        switch (new_image_layout)
        {
            case vk::ImageLayout::eColorAttachmentOptimal:
                destination_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
                break;
            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                destination_stage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
                break;
            case vk::ImageLayout::eGeneral:
                destination_stage = vk::PipelineStageFlagBits::eHost;
                break;
            case vk::ImageLayout::ePresentSrcKHR:
                destination_stage = vk::PipelineStageFlagBits::eBottomOfPipe;
                break;
            case vk::ImageLayout::eShaderReadOnlyOptimal:
                destination_stage = vk::PipelineStageFlagBits::eFragmentShader;
                break;
            case vk::ImageLayout::eTransferDstOptimal:
            case vk::ImageLayout::eTransferSrcOptimal:
                destination_stage = vk::PipelineStageFlagBits::eTransfer;
                break;
            default:
                assert(false);
                break;
        }

        vk::ImageAspectFlags aspect_mask;
        if (new_image_layout == vk::ImageLayout::eDepthStencilAttachmentOptimal)
        {
            aspect_mask = vk::ImageAspectFlagBits::eDepth;
            if (format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint)
            {
                aspect_mask |= vk::ImageAspectFlagBits::eStencil;
            }
        }
        else
        {
            aspect_mask = vk::ImageAspectFlagBits::eColor;
        }

        vk::ImageSubresourceRange image_subresource_range(aspect_mask, 0, 1, 0, 1);

        vk::ImageMemoryBarrier image_memory_barrier(source_access_mask,       /* srcAccessMask */
                                                    destination_access_mask,  /* dstAccessMask */
                                                    old_image_layout,         /* oldLayout */
                                                    new_image_layout,         /* newLayout */
                                                    VK_QUEUE_FAMILY_IGNORED,  /* srcQueueFamilyIndex */
                                                    VK_QUEUE_FAMILY_IGNORED,  /* dstQueueFamilyIndex */
                                                    image,                    /* image */
                                                    image_subresource_range); /* subresourceRange */

        command_buffer.pipelineBarrier(source_stage,          /* srcStageMask */
                                       destination_stage,     /* dstStageMask */
                                       {},                    /* dependencyFlags */
                                       nullptr,               /* pMemoryBarriers */
                                       nullptr,               /* pBufferMemoryBarriers */
                                       image_memory_barrier); /* pImageMemoryBarriers */
    }

    vk::raii::Pipeline
    MakeGraphicsPipeline(vk::raii::Device const&                             device,
                         vk::raii::PipelineCache const&                      pipeline_cache,
                         vk::raii::ShaderModule const&                       vertex_shader_module,
                         vk::SpecializationInfo const*                       vertex_shader_specialization_info,
                         vk::raii::ShaderModule const&                       fragment_shader_module,
                         vk::SpecializationInfo const*                       fragment_shader_specialization_info,
                         uint32_t                                            vertex_stride,
                         std::vector<std::pair<vk::Format, uint32_t>> const& vertex_input_attribute_format_offset,
                         vk::FrontFace                                       front_face,
                         bool                                                depth_buffered,
                         vk::raii::PipelineLayout const&                     pipeline_layout,
                         vk::raii::RenderPass const&                         render_pass)
    {
        std::array<vk::PipelineShaderStageCreateInfo, 2> pipeline_shader_stage_create_infos = {
            vk::PipelineShaderStageCreateInfo(
                {}, vk::ShaderStageFlagBits::eVertex, *vertex_shader_module, "main", vertex_shader_specialization_info),
            vk::PipelineShaderStageCreateInfo({},
                                              vk::ShaderStageFlagBits::eFragment,
                                              *fragment_shader_module,
                                              "main",
                                              fragment_shader_specialization_info)};

        std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;
        vk::PipelineVertexInputStateCreateInfo           pipeline_vertex_input_state_create_info;
        vk::VertexInputBindingDescription                vertex_input_binding_description(0, vertex_stride);

        if (0 < vertex_stride)
        {
            vertex_input_attribute_descriptions.reserve(vertex_input_attribute_format_offset.size());
            for (uint32_t i = 0; i < vertex_input_attribute_format_offset.size(); i++)
            {
                vertex_input_attribute_descriptions.emplace_back(i,
                                                                 0,
                                                                 vertex_input_attribute_format_offset[i].first,
                                                                 vertex_input_attribute_format_offset[i].second);
            }
            pipeline_vertex_input_state_create_info.setVertexBindingDescriptions(vertex_input_binding_description);
            pipeline_vertex_input_state_create_info.setVertexAttributeDescriptions(vertex_input_attribute_descriptions);
        }

        vk::PipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_create_info(
            vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList);

        vk::PipelineViewportStateCreateInfo pipeline_viewport_state_create_info(
            vk::PipelineViewportStateCreateFlags(), 1, nullptr, 1, nullptr);

        vk::PipelineRasterizationStateCreateInfo pipeline_rasterization_state_create_info(
            vk::PipelineRasterizationStateCreateFlags(),
            false,
            false,
            vk::PolygonMode::eFill,
            vk::CullModeFlagBits::eBack,
            front_face,
            false,
            0.0f,
            0.0f,
            0.0f,
            1.0f);

        vk::PipelineMultisampleStateCreateInfo pipeline_multisample_state_create_info({}, vk::SampleCountFlagBits::e1);

        vk::StencilOpState stencil_op_state(
            vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways);
        vk::PipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_create_info(
            vk::PipelineDepthStencilStateCreateFlags(),
            depth_buffered,
            depth_buffered,
            vk::CompareOp::eLessOrEqual,
            false,
            false,
            stencil_op_state,
            stencil_op_state);

        vk::ColorComponentFlags color_component_flags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
        vk::PipelineColorBlendAttachmentState pipeline_color_blend_attachment_state(false,
                                                                                    vk::BlendFactor::eOne,
                                                                                    vk::BlendFactor::eZero,
                                                                                    vk::BlendOp::eAdd,
                                                                                    vk::BlendFactor::eOne,
                                                                                    vk::BlendFactor::eZero,
                                                                                    vk::BlendOp::eAdd,
                                                                                    color_component_flags);
        vk::PipelineColorBlendStateCreateInfo pipeline_color_blend_state_create_info(
            vk::PipelineColorBlendStateCreateFlags(),
            false,
            vk::LogicOp::eNoOp,
            pipeline_color_blend_attachment_state,
            {{1.0f, 1.0f, 1.0f, 1.0f}});

        std::array<vk::DynamicState, 2>    dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
        vk::PipelineDynamicStateCreateInfo pipeline_dynamic_state_create_info(vk::PipelineDynamicStateCreateFlags(),
                                                                              dynamic_states);

        vk::GraphicsPipelineCreateInfo graphics_pipeline_create_info(vk::PipelineCreateFlags(),
                                                                     pipeline_shader_stage_create_infos,
                                                                     &pipeline_vertex_input_state_create_info,
                                                                     &pipeline_input_assembly_state_create_info,
                                                                     nullptr,
                                                                     &pipeline_viewport_state_create_info,
                                                                     &pipeline_rasterization_state_create_info,
                                                                     &pipeline_multisample_state_create_info,
                                                                     &pipeline_depth_stencil_state_create_info,
                                                                     &pipeline_color_blend_state_create_info,
                                                                     &pipeline_dynamic_state_create_info,
                                                                     *pipeline_layout,
                                                                     *render_pass);

        return vk::raii::Pipeline(device, pipeline_cache, graphics_pipeline_create_info);
    }

} // namespace Meow
