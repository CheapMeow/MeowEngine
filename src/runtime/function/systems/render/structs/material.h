#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>

namespace Meow
{
    struct Material
    {
        vk::raii::PipelineLayout pipeline_layout   = nullptr;
        vk::raii::Pipeline       graphics_pipeline = nullptr;

        Material() {}

        Material(vk::raii::Device const&              logical_device,
                 vk::raii::DescriptorSetLayout const& descriptor_set_layout,
                 vk::raii::RenderPass const&          render_pass);

        void Bind(vk::raii::CommandBuffer const& command_buffer, vk::raii::DescriptorSet const& descriptor_set);
    };

} // namespace Meow
