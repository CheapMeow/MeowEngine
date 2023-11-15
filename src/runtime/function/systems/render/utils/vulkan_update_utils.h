#pragma once

#include "function/systems/render/structs/texture_data.h"

namespace Meow
{
    void UpdateDescriptorSets(
        vk::raii::Device const&        device,
        vk::raii::DescriptorSet const& descriptor_set,
        std::vector<
            std::tuple<vk::DescriptorType, vk::raii::Buffer const&, vk::DeviceSize, vk::raii::BufferView const*>> const&
                           buffer_data,
        TextureData const& texture_data,
        uint32_t           binding_offset = 0);

    void UpdateDescriptorSets(
        vk::raii::Device const&        device,
        vk::raii::DescriptorSet const& descriptor_set,
        std::vector<
            std::tuple<vk::DescriptorType, vk::raii::Buffer const&, vk::DeviceSize, vk::raii::BufferView const*>> const&
                                        buffer_data,
        std::vector<TextureData> const& texture_data,
        uint32_t                        binding_offset = 0);
} // namespace Meow
