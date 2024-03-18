#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "function/render/structs/buffer_data.h"
#include "function/render/structs/texture_data.hpp"
#include "function/render/structs/ubo_data.h"

#include <cstdint>

namespace Meow
{
    struct Material
    {
        vk::raii::DescriptorSetLayout descriptor_set_layout = nullptr;
        vk::raii::DescriptorSet       descriptor_set        = nullptr;
        BufferData                    uniform_buffer_data   = nullptr;
        vk::raii::PipelineLayout      pipeline_layout       = nullptr;
        vk::raii::Pipeline            graphics_pipeline     = nullptr;

        std::shared_ptr<TextureData> diffuse_texture;

        Material() {}

        Material(vk::raii::PhysicalDevice const& gpu,
                 vk::raii::Device const&         logical_device,
                 vk::raii::DescriptorPool const& descriptor_pool,
                 vk::raii::RenderPass const&     render_pass,
                 std::string                     vert_shader_file_path,
                 std::string                     frag_shader_file_path,
                 std::shared_ptr<TextureData>    diffuse_texture_);

        void CreateDescriptorSetLayout(vk::raii::Device const& logical_device);

        void CreateUniformBuffer(vk::raii::PhysicalDevice const& gpu, vk::raii::Device const& logical_device);

        void CreateDescriptorSet(vk::raii::Device const&         logical_device,
                                 vk::raii::DescriptorPool const& descriptor_pool);

        void Bind(vk::raii::CommandBuffer const& command_buffer);

        void UpdateUniformBuffer(UBOData ubo_data);
    };

} // namespace Meow
