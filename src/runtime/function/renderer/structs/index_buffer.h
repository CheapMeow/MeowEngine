#pragma once

#include "core/base/non_copyable.h"
#include "function/renderer/utils/vulkan_hpp_utils.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <string>

namespace Meow
{
    struct IndexBuffer : NonCopyable
    {
        vk::Meow::BufferData buffer_data;
        uint32_t             count = 0;
        vk::IndexType        type  = vk::IndexType::eUint16;

        template<typename T>
        IndexBuffer(vk::raii::PhysicalDevice const& physical_device,
                    vk::raii::Device const&         device,
                    vk::DeviceSize                  device_size,
                    vk::MemoryPropertyFlags         property_flags = vk::MemoryPropertyFlagBits::eHostVisible |
                                                             vk::MemoryPropertyFlagBits::eHostCoherent,
                    T const*      p_data = nullptr,
                    uint32_t      _count = 0,
                    vk::IndexType _type  = vk::IndexType::eUint16
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                    ,
                    std::string primitive_name = "Default Primitive Name"
#endif
                    )
            : buffer_data(physical_device, device, device_size, vk::BufferUsageFlagBits::eIndexBuffer, property_flags)
            , count(_count)
            , type(_type)
        {
            vk::Meow::CopyToDevice(buffer_data.device_memory, p_data, count);

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            std::string                     object_name = std::format("{} {}", primitive_name, "Index Buffer");
            vk::DebugUtilsObjectNameInfoEXT name_info   = {
                vk::ObjectType::eBuffer, vk::Meow::GetVulkanHandle(*buffer_data.buffer), object_name.c_str(), nullptr};
            device.setDebugUtilsObjectNameEXT(name_info);

            object_name = std::format("{} {}", primitive_name, "Index Buffer Device Memory");
            name_info   = {vk::ObjectType::eDeviceMemory,
                           vk::Meow::GetVulkanHandle(*buffer_data.device_memory),
                           object_name.c_str(),
                           nullptr};
            device.setDebugUtilsObjectNameEXT(name_info);
#endif
        }

        void BindDraw(const vk::raii::CommandBuffer& cmd_buffer) const;
    };
} // namespace Meow