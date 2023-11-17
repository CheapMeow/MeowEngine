#include "texture_data.hpp"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    TextureData::TextureData(vk::raii::PhysicalDevice const& physical_device,
                             vk::raii::Device const&         device,
                             vk::Extent2D const&             extent_,
                             vk::ImageUsageFlags             usage_flags,
                             vk::FormatFeatureFlags          format_feature_flags,
                             bool                            anisotropy_enable,
                             bool                            force_staging)
        : format(vk::Format::eR8G8B8A8Unorm)
        , extent(extent_)
        , sampler(device,
                  {{},
                   vk::Filter::eLinear,
                   vk::Filter::eLinear,
                   vk::SamplerMipmapMode::eLinear,
                   vk::SamplerAddressMode::eRepeat,
                   vk::SamplerAddressMode::eRepeat,
                   vk::SamplerAddressMode::eRepeat,
                   0.0f,
                   anisotropy_enable,
                   16.0f,
                   false,
                   vk::CompareOp::eNever,
                   0.0f,
                   0.0f,
                   vk::BorderColor::eFloatOpaqueBlack})
    {
        vk::FormatProperties format_properties = physical_device.getFormatProperties(format);

        format_feature_flags |= vk::FormatFeatureFlagBits::eSampledImage;
        need_staging =
            force_staging || ((format_properties.linearTilingFeatures & format_feature_flags) != format_feature_flags);
        vk::ImageTiling         image_tiling;
        vk::ImageLayout         initial_layout;
        vk::MemoryPropertyFlags requirements;
        if (need_staging)
        {
            assert((format_properties.optimalTilingFeatures & format_feature_flags) == format_feature_flags);
            staging_buffer_data = BufferData(
                physical_device, device, extent.width * extent.height * 4, vk::BufferUsageFlagBits::eTransferSrc);
            image_tiling = vk::ImageTiling::eOptimal;
            usage_flags |= vk::ImageUsageFlagBits::eTransferDst;
            initial_layout = vk::ImageLayout::eUndefined;
        }
        else
        {
            image_tiling   = vk::ImageTiling::eLinear;
            initial_layout = vk::ImageLayout::ePreinitialized;
            requirements   = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
        }
        image_data = ImageData(physical_device,
                               device,
                               format,
                               extent,
                               image_tiling,
                               usage_flags | vk::ImageUsageFlagBits::eSampled,
                               initial_layout,
                               requirements,
                               vk::ImageAspectFlagBits::eColor);
    }

    void TextureData::TransitLayout(vk::raii::CommandBuffer const& command_buffer)
    {
        if (need_staging)
        {
            // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
            SetImageLayout(command_buffer,
                           *image_data.image,
                           image_data.format,
                           vk::ImageLayout::eUndefined,
                           vk::ImageLayout::eTransferDstOptimal);
            vk::BufferImageCopy copy_region(0,
                                            extent.width,
                                            extent.height,
                                            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                            vk::Offset3D(0, 0, 0),
                                            vk::Extent3D(extent, 1));
            command_buffer.copyBufferToImage(
                *staging_buffer_data.buffer, *image_data.image, vk::ImageLayout::eTransferDstOptimal, copy_region);
            // Set the layout for the texture image from eTransferDstOptimal to eShaderReadOnlyOptimal
            SetImageLayout(command_buffer,
                           *image_data.image,
                           image_data.format,
                           vk::ImageLayout::eTransferDstOptimal,
                           vk::ImageLayout::eShaderReadOnlyOptimal);
        }
        else
        {
            // If we can use the linear tiled image as a texture, just do it
            SetImageLayout(command_buffer,
                           *image_data.image,
                           image_data.format,
                           vk::ImageLayout::ePreinitialized,
                           vk::ImageLayout::eShaderReadOnlyOptimal);
        }
    }

    bool TextureData::LoadImageFromFile(vk::raii::CommandBuffer const& command_buffer, std::string const& filepath)
    {
        void* data = need_staging ?
                         staging_buffer_data.device_memory.mapMemory(
                             0, staging_buffer_data.buffer.getMemoryRequirements().size) :
                         image_data.device_memory.mapMemory(0, image_data.image.getMemoryRequirements().size);
        g_runtime_global_context.file_system->ReadImageFileToPtr(filepath, static_cast<uint8_t*>(data));
        need_staging ? staging_buffer_data.device_memory.unmapMemory() : image_data.device_memory.unmapMemory();

        TransitLayout(command_buffer);

        return true;
    }
} // namespace Meow