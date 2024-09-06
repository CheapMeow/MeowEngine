#include "image_data.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    void ImageData::SetImageLayout(vk::raii::CommandBuffer const& command_buffer,
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

        vk::ImageSubresourceRange image_subresource_range(aspect_mask, 0, 1, 0, 1);

        vk::ImageMemoryBarrier image_memory_barrier(source_access_mask,       /* srcAccessMask */
                                                    destination_access_mask,  /* dstAccessMask */
                                                    old_image_layout,         /* oldLayout */
                                                    new_image_layout,         /* newLayout */
                                                    VK_QUEUE_FAMILY_IGNORED,  /* srcQueueFamilyIndex */
                                                    VK_QUEUE_FAMILY_IGNORED,  /* dstQueueFamilyIndex */
                                                    *image,                   /* image */
                                                    image_subresource_range); /* subresourceRange */

        command_buffer.pipelineBarrier(source_stage,          /* srcStageMask */
                                       destination_stage,     /* dstStageMask */
                                       {},                    /* dependencyFlags */
                                       nullptr,               /* pMemoryBarriers */
                                       nullptr,               /* pBufferMemoryBarriers */
                                       image_memory_barrier); /* pImageMemoryBarriers */
    }

    void ImageData::TransitLayout(vk::raii::CommandBuffer const& command_buffer)
    {
        command_buffer.begin({});

        if (need_staging)
        {
            // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
            SetImageLayout(command_buffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
            vk::BufferImageCopy copy_region(0,
                                            extent.width,
                                            extent.height,
                                            vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                            vk::Offset3D(0, 0, 0),
                                            vk::Extent3D(extent, 1));
            command_buffer.copyBufferToImage(
                *staging_buffer_data.buffer, *image, vk::ImageLayout::eTransferDstOptimal, copy_region);
            // Set the layout for the texture image from eTransferDstOptimal to eShaderReadOnlyOptimal
            SetImageLayout(
                command_buffer, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
        }
        else
        {
            // If we can use the linear tiled image as a texture, just do it
            SetImageLayout(command_buffer, vk::ImageLayout::ePreinitialized, vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        command_buffer.end();
    }

    std::shared_ptr<ImageData> ImageData::CreateDepthBuffer(vk::raii::PhysicalDevice const& physical_device,
                                                            vk::raii::Device const&         device,
                                                            vk::raii::CommandPool const&    command_pool,
                                                            vk::raii::Queue const&          queue,
                                                            vk::Format                      format,
                                                            vk::Extent2D const&             extent)
    {
        std::shared_ptr<ImageData> image_data_ptr = std::make_shared<ImageData>(nullptr);

        // Create Depth Buffer

        vk::ImageTiling         image_tiling   = vk::ImageTiling::eOptimal;
        vk::ImageUsageFlags     usage_flags    = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        vk::ImageLayout         initial_layout = vk::ImageLayout::eUndefined;
        vk::MemoryPropertyFlags requirements   = vk::MemoryPropertyFlagBits::eDeviceLocal;
        vk::ImageAspectFlags    aspect_mask    = vk::ImageAspectFlagBits::eDepth;

        image_data_ptr->format      = format;
        image_data_ptr->extent      = extent;
        image_data_ptr->aspect_mask = aspect_mask;

        // doesn't need sampler

        // Create Image

        vk::ImageCreateInfo image_create_info(vk::ImageCreateFlags(),
                                              vk::ImageType::e2D,
                                              format,
                                              vk::Extent3D(extent, 1),
                                              1,
                                              1,
                                              vk::SampleCountFlagBits::e1,
                                              image_tiling,
                                              usage_flags | vk::ImageUsageFlagBits::eSampled,
                                              vk::SharingMode::eExclusive,
                                              {},
                                              initial_layout);
        image_data_ptr->image = vk::raii::Image(device, image_create_info);

        image_data_ptr->device_memory = AllocateDeviceMemory(
            device, physical_device.getMemoryProperties(), image_data_ptr->image.getMemoryRequirements(), requirements);
        image_data_ptr->image.bindMemory(*image_data_ptr->device_memory, 0);
        image_data_ptr->image_view = vk::raii::ImageView(
            device,
            vk::ImageViewCreateInfo(
                {}, *image_data_ptr->image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        // Transit Layout
        OneTimeSubmit(device, command_pool, queue, [&](vk::raii::CommandBuffer const& command_buffer) {
            image_data_ptr->SetImageLayout(
                command_buffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        });

        return image_data_ptr;
    }

    std::shared_ptr<ImageData> ImageData::CreateTexture(vk::raii::PhysicalDevice const& physical_device,
                                                        vk::raii::Device const&         device,
                                                        vk::Format                      format,
                                                        vk::Extent2D const&             extent,
                                                        vk::ImageUsageFlags             usage_flags,
                                                        vk::ImageAspectFlags            aspect_mask,
                                                        vk::FormatFeatureFlags          format_feature_flags,
                                                        bool                            anisotropy_enable,
                                                        bool                            force_staging)
    {
        std::shared_ptr<ImageData> image_data_ptr = std::make_shared<ImageData>(nullptr);

        // Create Texture

        image_data_ptr->format      = format;
        image_data_ptr->extent      = extent;
        image_data_ptr->aspect_mask = aspect_mask;

        vk::SamplerCreateInfo sampler_create_info({},
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
                                                  vk::BorderColor::eFloatOpaqueBlack);
        image_data_ptr->sampler = vk::raii::Sampler(device, sampler_create_info);

        vk::FormatProperties format_properties = physical_device.getFormatProperties(format);

        format_feature_flags |= vk::FormatFeatureFlagBits::eSampledImage;
        image_data_ptr->need_staging =
            force_staging || ((format_properties.linearTilingFeatures & format_feature_flags) != format_feature_flags);
        vk::ImageTiling         image_tiling;
        vk::ImageLayout         initial_layout;
        vk::MemoryPropertyFlags requirements;
        if (image_data_ptr->need_staging)
        {
            assert((format_properties.optimalTilingFeatures & format_feature_flags) == format_feature_flags);
            image_data_ptr->staging_buffer_data = BufferData(
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

        // Create Image

        vk::ImageCreateInfo image_create_info(vk::ImageCreateFlags(),
                                              vk::ImageType::e2D,
                                              format,
                                              vk::Extent3D(extent, 1),
                                              1,
                                              1,
                                              vk::SampleCountFlagBits::e1,
                                              image_tiling,
                                              usage_flags | vk::ImageUsageFlagBits::eSampled,
                                              vk::SharingMode::eExclusive,
                                              {},
                                              initial_layout);
        image_data_ptr->image = vk::raii::Image(device, image_create_info);

        image_data_ptr->device_memory = AllocateDeviceMemory(
            device, physical_device.getMemoryProperties(), image_data_ptr->image.getMemoryRequirements(), requirements);
        image_data_ptr->image.bindMemory(*image_data_ptr->device_memory, 0);
        image_data_ptr->image_view = vk::raii::ImageView(
            device,
            vk::ImageViewCreateInfo(
                {}, *image_data_ptr->image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        return image_data_ptr;
    }

    std::shared_ptr<ImageData> ImageData::CreateTexture(vk::raii::PhysicalDevice const& physical_device,
                                                        vk::raii::Device const&         device,
                                                        vk::raii::CommandPool const&    command_pool,
                                                        vk::raii::Queue const&          queue,
                                                        std::string const&              file_path,
                                                        vk::Format                      format,
                                                        vk::ImageUsageFlags             usage_flags,
                                                        vk::ImageAspectFlags            aspect_mask,
                                                        vk::FormatFeatureFlags          format_feature_flags,
                                                        bool                            anisotropy_enable,
                                                        bool                            force_staging)
    {
        auto [width, height] = g_runtime_global_context.file_system->GetImageFileWidthHeight(file_path);
        if (width == 0 || height == 0)
        {
            return nullptr;
        }

        std::shared_ptr<ImageData> image_data_ptr = ImageData::CreateTexture(physical_device,
                                                                             device,
                                                                             format,
                                                                             vk::Extent2D {width, height},
                                                                             usage_flags,
                                                                             aspect_mask,
                                                                             format_feature_flags,
                                                                             anisotropy_enable,
                                                                             force_staging);

        // Read image from file to device memory

        void* data = image_data_ptr->need_staging ?
                         image_data_ptr->staging_buffer_data.device_memory.mapMemory(
                             0, image_data_ptr->staging_buffer_data.buffer.getMemoryRequirements().size) :
                         image_data_ptr->device_memory.mapMemory(0, image_data_ptr->image.getMemoryRequirements().size);

        if (g_runtime_global_context.file_system->ReadImageFileToPtr(file_path, static_cast<uint8_t*>(data)) == 0)
            return nullptr;

        image_data_ptr->need_staging ? image_data_ptr->staging_buffer_data.device_memory.unmapMemory() :
                                       image_data_ptr->device_memory.unmapMemory();

        // Transit Layout

        OneTimeSubmit(device, command_pool, queue, [&](vk::raii::CommandBuffer const& command_buffer) {
            if (image_data_ptr->need_staging)
            {
                // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
                image_data_ptr->SetImageLayout(
                    command_buffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
                vk::BufferImageCopy copy_region(0,
                                                image_data_ptr->extent.width,
                                                image_data_ptr->extent.height,
                                                vk::ImageSubresourceLayers(aspect_mask, 0, 0, 1),
                                                vk::Offset3D(0, 0, 0),
                                                vk::Extent3D(image_data_ptr->extent, 1));
                command_buffer.copyBufferToImage(*image_data_ptr->staging_buffer_data.buffer,
                                                 *image_data_ptr->image,
                                                 vk::ImageLayout::eTransferDstOptimal,
                                                 copy_region);
                // Set the layout for the texture image from eTransferDstOptimal to eShaderReadOnlyOptimal
                image_data_ptr->SetImageLayout(
                    command_buffer, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
            }
            else
            {
                // If we can use the linear tiled image as a texture, just do it
                image_data_ptr->SetImageLayout(
                    command_buffer, vk::ImageLayout::ePreinitialized, vk::ImageLayout::eShaderReadOnlyOptimal);
            }
        });

        return image_data_ptr;
    }

    std::shared_ptr<ImageData> ImageData::CreateAttachment(vk::raii::PhysicalDevice const& physical_device,
                                                           vk::raii::Device const&         device,
                                                           vk::raii::CommandPool const&    command_pool,
                                                           vk::raii::Queue const&          queue,
                                                           vk::Format                      format,
                                                           vk::Extent2D const&             extent,
                                                           vk::ImageUsageFlags             usage_flags,
                                                           vk::ImageAspectFlags            aspect_mask,
                                                           vk::FormatFeatureFlags          format_feature_flags,
                                                           bool                            anisotropy_enable)
    {
        std::shared_ptr<ImageData> image_data_ptr = std::make_shared<ImageData>(nullptr);

        // Create Attachment

        vk::ImageTiling         image_tiling   = vk::ImageTiling::eOptimal;
        vk::ImageLayout         initial_layout = vk::ImageLayout::eUndefined;
        vk::MemoryPropertyFlags requirements   = vk::MemoryPropertyFlagBits::eDeviceLocal;

        image_data_ptr->format      = format;
        image_data_ptr->extent      = extent;
        image_data_ptr->aspect_mask = aspect_mask;

        // doesn't need sampler

        // Create Image

        vk::ImageCreateInfo image_create_info(vk::ImageCreateFlags(),
                                              vk::ImageType::e2D,
                                              format,
                                              vk::Extent3D(extent, 1),
                                              1,
                                              1,
                                              vk::SampleCountFlagBits::e1,
                                              image_tiling,
                                              usage_flags | vk::ImageUsageFlagBits::eSampled,
                                              vk::SharingMode::eExclusive,
                                              {},
                                              initial_layout);
        image_data_ptr->image = vk::raii::Image(device, image_create_info);

        image_data_ptr->device_memory = AllocateDeviceMemory(
            device, physical_device.getMemoryProperties(), image_data_ptr->image.getMemoryRequirements(), requirements);
        image_data_ptr->image.bindMemory(*image_data_ptr->device_memory, 0);
        image_data_ptr->image_view = vk::raii::ImageView(
            device,
            vk::ImageViewCreateInfo(
                {}, *image_data_ptr->image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        // Transit Layout
        OneTimeSubmit(device, command_pool, queue, [&](vk::raii::CommandBuffer const& command_buffer) {
            if (aspect_mask & vk::ImageAspectFlagBits::eColor)
                image_data_ptr->SetImageLayout(
                    command_buffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);
            else if (aspect_mask & vk::ImageAspectFlagBits::eDepth)
                image_data_ptr->SetImageLayout(
                    command_buffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);
        });

        return image_data_ptr;
    }
} // namespace Meow
