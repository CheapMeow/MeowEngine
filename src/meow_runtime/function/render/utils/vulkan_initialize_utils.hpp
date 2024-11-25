#pragma once

#include "pch.h"

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    template<typename TargetType, typename SourceType>
    VULKAN_HPP_INLINE TargetType checked_cast(SourceType value)
    {
        static_assert(sizeof(TargetType) <= sizeof(SourceType), "No need to cast from smaller to larger type!");
        static_assert(std::numeric_limits<SourceType>::is_integer, "Only integer types supported!");
        static_assert(!std::numeric_limits<SourceType>::is_signed, "Only unsigned types supported!");
        static_assert(std::numeric_limits<TargetType>::is_integer, "Only integer types supported!");
        static_assert(!std::numeric_limits<TargetType>::is_signed, "Only unsigned types supported!");
        assert(value <= std::numeric_limits<TargetType>::max());
        return static_cast<TargetType>(value);
    }

    template<typename T>
    uint64_t GetVulkanHandle(T const& cpp_handle)
    {
        return reinterpret_cast<uint64_t>(static_cast<const T::CType>(cpp_handle));
    }

    std::vector<const char*>
    GetRequiredInstanceExtensions(std::vector<const char*> const& required_instance_extensions);

    bool ValidateExtensions(const std::vector<const char*>&             required,
                            const std::vector<vk::ExtensionProperties>& available);

    bool ValidateLayers(const std::vector<const char*>& required, const std::vector<vk::LayerProperties>& available);

    std::vector<const char*>
    GetOptimalValidationLayers(const std::vector<vk::LayerProperties>& supported_instance_layers);

    VKAPI_ATTR VkBool32 VKAPI_CALL
    DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                void* /*pUserData*/);

    vk::DebugUtilsMessengerCreateInfoEXT MakeDebugUtilsMessengerCreateInfoEXT();

    void LogVulkanAPIVersion(const uint32_t api_version);

    void LogVulkanDevice(const VkPhysicalDeviceProperties&         physical_device_properties,
                         const std::vector<VkExtensionProperties>& extension_properties);

    uint32_t ScorePhysicalDevice(const vk::raii::PhysicalDevice& device,
                                 const std::vector<const char*>& required_device_extensions);

    uint32_t FindGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queue_family_properties);

    std::pair<uint32_t, uint32_t>
    FindGraphicsAndPresentQueueFamilyIndex(vk::raii::PhysicalDevice const& physical_device,
                                           vk::raii::SurfaceKHR const&     surface);

    vk::SurfaceFormatKHR PickSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& formats);

    vk::PresentModeKHR PickPresentMode(std::vector<vk::PresentModeKHR> const& present_modes);

    uint32_t FindMemoryType(vk::PhysicalDeviceMemoryProperties const& memory_properties,
                            uint32_t                                  type_bits,
                            vk::MemoryPropertyFlags                   requirements_mask);

    vk::raii::DeviceMemory AllocateDeviceMemory(vk::raii::Device const&                   device,
                                                vk::PhysicalDeviceMemoryProperties const& memory_properties,
                                                vk::MemoryRequirements const&             memory_requirements,
                                                vk::MemoryPropertyFlags                   memory_property_flags);

    template<typename T>
    void CopyToDevice(vk::raii::DeviceMemory const& device_memory,
                      T const*                      p_data,
                      size_t                        count,
                      vk::DeviceSize                stride = sizeof(T))
    {
        assert(sizeof(T) <= stride);
        uint8_t* device_data = static_cast<uint8_t*>(device_memory.mapMemory(0, count * stride));
        if (stride == sizeof(T))
        {
            memcpy(device_data, p_data, count * sizeof(T));
        }
        else
        {
            for (size_t i = 0; i < count; i++)
            {
                memcpy(device_data, &p_data[i], sizeof(T));
                device_data += stride;
            }
        }
        device_memory.unmapMemory();
    }

    template<typename T>
    void CopyToDevice(vk::raii::DeviceMemory const& device_memory, T const& data)
    {
        CopyToDevice<T>(device_memory, &data, 1);
    }

    template<typename Func>
    void OneTimeSubmit(vk::raii::Device const&      logical_device,
                       vk::raii::CommandPool const& command_pool,
                       vk::raii::Queue const&       queue,
                       Func const&                  func)
    {
        vk::CommandBufferAllocateInfo command_buffer_allocate_info = {
            .commandPool        = *command_pool,
            .level              = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1,
        };
        vk::raii::CommandBuffer command_buffer =
            std::move(vk::raii::CommandBuffers(logical_device, command_buffer_allocate_info).front());
        command_buffer.begin(vk::CommandBufferBeginInfo {.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        func(command_buffer);
        command_buffer.end();
        vk::SubmitInfo submit_info = {
            .commandBufferCount = 1,
            .pCommandBuffers    = &(*command_buffer),
        };
        queue.submit(submit_info, nullptr);
        queue.waitIdle();
    }

    /**
     * @brief vk::raii::SwapchainKHR::acquireNextImageKHR without exceptions
     */
    std::pair<vk::Result, uint32_t> SwapchainNextImageWrapper(const vk::raii::SwapchainKHR& swapchain,
                                                              uint64_t                      timeout,
                                                              vk::Semaphore                 semaphore,
                                                              vk::Fence                     fence = {});

    /**
     * @brief vk::raii::Queue::presentKHR without exceptions
     */
    vk::Result QueuePresentWrapper(const vk::raii::Queue& queue, const vk::PresentInfoKHR& present_info);
} // namespace Meow
