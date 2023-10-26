#pragma once

#include "core/base/non_copyable.h"
#include "function/renderer/structs/vertex_attribute.h"
#include "function/renderer/utils/vulkan_hpp_utils.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace Meow
{
    struct VertexBuffer : NonCopyable
    {
        vk::Meow::BufferData         buffer_data;
        uint32_t                     count = 0;
        std::vector<VertexAttribute> attributes;

        VertexBuffer(vk::raii::PhysicalDevice const& physical_device,
                     vk::raii::Device const&         device,
                     vk::DeviceSize                  size,
                     vk::MemoryPropertyFlags         property_flags = vk::MemoryPropertyFlagBits::eHostVisible |
                                                              vk::MemoryPropertyFlagBits::eHostCoherent,
                     float const*                 p_data      = nullptr,
                     uint32_t                     _count      = 0,
                     std::vector<VertexAttribute> _attributes = {})
            : buffer_data(physical_device, device, size, vk::BufferUsageFlagBits::eVertexBuffer, property_flags)
            , count(_count)
            , attributes(_attributes)
        {
            vk::Meow::CopyToDevice(buffer_data.device_memory, p_data, count);
        }

        std::vector<VkVertexInputAttributeDescription> GetInputAttributes() const;

        VkVertexInputBindingDescription GetInputBinding() const;

        void Bind(const vk::raii::CommandBuffer& cmd_buffer) const;
    };
} // namespace Meow