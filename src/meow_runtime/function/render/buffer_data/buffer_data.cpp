#include "buffer_data.h"

#include "function/global/runtime_context.h"
#include "function/render/utils/vulkan_debug_utils.h"

namespace Meow
{
    BufferData::BufferData(vk::raii::PhysicalDevice const& physical_device,
                           vk::raii::Device const&         logical_device,
                           vk::DeviceSize                  size,
                           vk::BufferUsageFlags            usage,
                           vk::MemoryPropertyFlags         property_flags)
        : buffer(logical_device, vk::BufferCreateInfo({}, size, usage))
        , device_size(size)
        , usage_flags(usage)
        , property_flags(property_flags)
    {
        device_memory = AllocateDeviceMemory(
            logical_device, physical_device.getMemoryProperties(), buffer.getMemoryRequirements(), property_flags);
        buffer.bindMemory(*device_memory, 0);
    }

    BufferData::BufferData(BufferData&& rhs)
        : device_memory(std::exchange(rhs.device_memory, nullptr))
        , buffer(std::exchange(rhs.buffer, nullptr))
        , device_size(rhs.device_size)
        , usage_flags(rhs.usage_flags)
        , property_flags(rhs.property_flags)
    {}

    BufferData& BufferData::operator=(BufferData&& rhs)
    {
        if (this != &rhs)
        {
            clear();

            device_memory  = std::exchange(rhs.device_memory, nullptr);
            buffer         = std::exchange(rhs.buffer, nullptr);
            device_size    = rhs.device_size;
            usage_flags    = rhs.usage_flags;
            property_flags = rhs.property_flags;
        }
        return *this;
    }

    void BufferData::clear()
    {
        device_memory = nullptr;
        buffer        = nullptr;
    }

    void BufferData::SetDebugName(const std::string& debug_name)
    {
        if (debug_name.empty())
            return;

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        {
            vk::DebugUtilsObjectNameInfoEXT name_info = {
                vk::ObjectType::eBuffer, NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkBuffer, *buffer), debug_name.c_str()};
            logical_device.setDebugUtilsObjectNameEXT(name_info);
        }

        {
            vk::DebugUtilsObjectNameInfoEXT name_info = {
                vk::ObjectType::eDeviceMemory,
                NON_DISPATCHABLE_HANDLE_TO_UINT64_CAST(VkDeviceMemory, *device_memory),
                debug_name.c_str()};
            logical_device.setDebugUtilsObjectNameEXT(name_info);
        }
#endif
    }
} // namespace Meow