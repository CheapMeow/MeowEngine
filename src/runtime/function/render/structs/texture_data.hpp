#pragma once

#include "function/render/structs/buffer_data.h"
#include "function/render/structs/image_data.h"

namespace Meow
{
    struct TextureData
    {
        vk::Format        format;
        vk::Extent2D      extent;
        bool              need_staging;
        BufferData        staging_buffer_data = nullptr;
        ImageData         image_data          = nullptr;
        vk::raii::Sampler sampler             = nullptr;

        TextureData(vk::raii::PhysicalDevice const& physical_device,
                    vk::raii::Device const&         device,
                    vk::Format                      format_              = vk::Format::eR8G8B8A8Unorm,
                    vk::Extent2D const&             extent_              = {256, 256},
                    vk::ImageUsageFlags             usage_flags          = {},
                    vk::ImageAspectFlags            aspect_mask          = vk::ImageAspectFlagBits::eColor,
                    vk::FormatFeatureFlags          format_feature_flags = {},
                    bool                            anisotropy_enable    = false,
                    bool                            force_staging        = false);

        TextureData(std::nullptr_t) {}
        TextureData() {}

        void TransitLayout(vk::raii::CommandBuffer const& command_buffer);

        template<typename ImageGenerator>
        void LoadImageFromGenerator(vk::raii::CommandBuffer const& command_buffer,
                                    ImageGenerator const&          image_generator)
        {
            void* data = need_staging ?
                             staging_buffer_data.device_memory.mapMemory(
                                 0, staging_buffer_data.buffer.getMemoryRequirements().size) :
                             image_data.device_memory.mapMemory(0, image_data.image.getMemoryRequirements().size);
            image_generator(data, extent);
            need_staging ? staging_buffer_data.device_memory.unmapMemory() : image_data.device_memory.unmapMemory();

            TransitLayout(command_buffer);
        }

        static std::shared_ptr<TextureData>
        CreateFromFile(vk::raii::PhysicalDevice const& physical_device,
                       vk::raii::Device const&         device,
                       std::string const&              filepath,
                       vk::Format                      format               = vk::Format::eR8G8B8A8Unorm,
                       vk::ImageUsageFlags             usage_flags          = {},
                       vk::ImageAspectFlags            aspect_mask          = vk::ImageAspectFlagBits::eColor,
                       vk::FormatFeatureFlags          format_feature_flags = {},
                       bool                            anisotropy_enable    = false,
                       bool                            force_staging        = false);
    };
} // namespace Meow
