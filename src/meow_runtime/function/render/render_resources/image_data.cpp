#include "image_data.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    void ImageData::SetLayout(const vk::raii::CommandBuffer& command_buffer,
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

        layout = new_image_layout;
    }

    std::shared_ptr<ImageData> ImageData::CreateDepthBuffer(vk::Format format, const vk::Extent2D& extent)
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        auto image_data_ptr = std::make_shared<ImageData>(nullptr);

        // Create Depth Buffer

        auto                    image_tiling   = vk::ImageTiling::eOptimal;
        vk::ImageUsageFlags     usage_flags    = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        auto                    initial_layout = vk::ImageLayout::eUndefined;
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
        image_data_ptr->image = vk::raii::Image(logical_device, image_create_info);

        image_data_ptr->device_memory = AllocateDeviceMemory(logical_device,
                                                             physical_device.getMemoryProperties(),
                                                             image_data_ptr->image.getMemoryRequirements(),
                                                             requirements);
        image_data_ptr->image.bindMemory(*image_data_ptr->device_memory, 0);
        image_data_ptr->image_view = vk::raii::ImageView(
            logical_device,
            vk::ImageViewCreateInfo(
                {}, *image_data_ptr->image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        // Transit Layout
        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](const vk::raii::CommandBuffer& command_buffer) {
                          image_data_ptr->SetLayout(command_buffer,
                                                    vk::ImageLayout::eUndefined,
                                                    vk::ImageLayout::eDepthStencilAttachmentOptimal);
                      });

        return image_data_ptr;
    }

    std::shared_ptr<ImageData> ImageData::CreateTexture(const std::string&     file_path,
                                                        vk::Format             format,
                                                        vk::ImageUsageFlags    usage_flags,
                                                        vk::ImageAspectFlags   aspect_mask,
                                                        vk::FormatFeatureFlags format_feature_flags,
                                                        bool                   anisotropy_enable,
                                                        bool                   force_staging)
    {
        auto [width, height] = g_runtime_context.file_system->GetImageFileWidthHeight(file_path);
        if (width == 0 || height == 0)
        {
            return nullptr;
        }
        vk::Extent2D extent = {width, height};

        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        auto image_data_ptr = std::make_shared<ImageData>(nullptr);

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
        image_data_ptr->sampler = vk::raii::Sampler(logical_device, sampler_create_info);

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
            image_data_ptr->staging_buffer_data = BufferData(physical_device,
                                                             logical_device,
                                                             extent.width * extent.height * 4,
                                                             vk::BufferUsageFlagBits::eTransferSrc);
            image_tiling                        = vk::ImageTiling::eOptimal;
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
        image_data_ptr->image = vk::raii::Image(logical_device, image_create_info);

        image_data_ptr->device_memory = AllocateDeviceMemory(logical_device,
                                                             physical_device.getMemoryProperties(),
                                                             image_data_ptr->image.getMemoryRequirements(),
                                                             requirements);
        image_data_ptr->image.bindMemory(*image_data_ptr->device_memory, 0);
        image_data_ptr->image_view = vk::raii::ImageView(
            logical_device,
            vk::ImageViewCreateInfo(
                {}, *image_data_ptr->image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        // Read image from file to device memory

        void* data = image_data_ptr->need_staging ?
                         image_data_ptr->staging_buffer_data.device_memory.mapMemory(
                             0, image_data_ptr->staging_buffer_data.buffer.getMemoryRequirements().size) :
                         image_data_ptr->device_memory.mapMemory(0, image_data_ptr->image.getMemoryRequirements().size);

        if (g_runtime_context.file_system->ReadImageFileToPtr(file_path, static_cast<uint8_t*>(data)) == 0)
            return nullptr;

        image_data_ptr->need_staging ? image_data_ptr->staging_buffer_data.device_memory.unmapMemory() :
                                       image_data_ptr->device_memory.unmapMemory();

        // Transit Layout

        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](const vk::raii::CommandBuffer& command_buffer) {
                          if (image_data_ptr->need_staging)
                          {
                              // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
                              image_data_ptr->SetLayout(
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
                              image_data_ptr->SetLayout(command_buffer,
                                                        vk::ImageLayout::eTransferDstOptimal,
                                                        vk::ImageLayout::eShaderReadOnlyOptimal);
                          }
                          else
                          {
                              // If we can use the linear tiled image as a texture, just do it
                              image_data_ptr->SetLayout(command_buffer,
                                                        vk::ImageLayout::ePreinitialized,
                                                        vk::ImageLayout::eShaderReadOnlyOptimal);
                          }
                      });

        return image_data_ptr;
    }

    std::shared_ptr<ImageData> ImageData::CreateAttachment(vk::Format             format,
                                                           const vk::Extent2D&    extent,
                                                           vk::ImageUsageFlags    usage_flags,
                                                           vk::ImageAspectFlags   aspect_mask,
                                                           vk::FormatFeatureFlags format_feature_flags,
                                                           bool                   anisotropy_enable)
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        auto image_data_ptr = std::make_shared<ImageData>(nullptr);

        // Create Attachment

        auto                    image_tiling   = vk::ImageTiling::eOptimal;
        auto                    initial_layout = vk::ImageLayout::eUndefined;
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
        image_data_ptr->image = vk::raii::Image(logical_device, image_create_info);

        image_data_ptr->device_memory = AllocateDeviceMemory(logical_device,
                                                             physical_device.getMemoryProperties(),
                                                             image_data_ptr->image.getMemoryRequirements(),
                                                             requirements);
        image_data_ptr->image.bindMemory(*image_data_ptr->device_memory, 0);
        image_data_ptr->image_view = vk::raii::ImageView(
            logical_device,
            vk::ImageViewCreateInfo(
                {}, *image_data_ptr->image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        // Transit Layout
        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](const vk::raii::CommandBuffer& command_buffer) {
                          if (aspect_mask & vk::ImageAspectFlagBits::eColor)
                              image_data_ptr->SetLayout(command_buffer,
                                                        vk::ImageLayout::eUndefined,
                                                        vk::ImageLayout::eColorAttachmentOptimal);
                          else if (aspect_mask & vk::ImageAspectFlagBits::eDepth)
                              image_data_ptr->SetLayout(command_buffer,
                                                        vk::ImageLayout::eUndefined,
                                                        vk::ImageLayout::eDepthStencilAttachmentOptimal);
                      });

        return image_data_ptr;
    }

    std::shared_ptr<ImageData> ImageData::CreateRenderTarget(vk::Format             format,
                                                             const vk::Extent2D&    extent,
                                                             vk::ImageUsageFlags    usage_flags,
                                                             vk::ImageAspectFlags   aspect_mask,
                                                             vk::FormatFeatureFlags format_feature_flags,
                                                             bool                   anisotropy_enable)
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        auto image_data_ptr = std::make_shared<ImageData>(nullptr);

        // Create Attachment

        auto                    image_tiling   = vk::ImageTiling::eOptimal;
        auto                    initial_layout = vk::ImageLayout::eUndefined;
        vk::MemoryPropertyFlags requirements   = vk::MemoryPropertyFlagBits::eDeviceLocal;

        image_data_ptr->format      = format;
        image_data_ptr->extent      = extent;
        image_data_ptr->aspect_mask = aspect_mask;

        // need sampler

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
        image_data_ptr->sampler = vk::raii::Sampler(logical_device, sampler_create_info);

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
        image_data_ptr->image = vk::raii::Image(logical_device, image_create_info);

        image_data_ptr->device_memory = AllocateDeviceMemory(logical_device,
                                                             physical_device.getMemoryProperties(),
                                                             image_data_ptr->image.getMemoryRequirements(),
                                                             requirements);
        image_data_ptr->image.bindMemory(*image_data_ptr->device_memory, 0);
        image_data_ptr->image_view = vk::raii::ImageView(
            logical_device,
            vk::ImageViewCreateInfo(
                {}, *image_data_ptr->image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        // Transit Layout
        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](const vk::raii::CommandBuffer& command_buffer) {
                          image_data_ptr->SetLayout(
                              command_buffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal);
                      });

        return image_data_ptr;
    }

    std::shared_ptr<ImageData> ImageData::CreateCubemap(const std::vector<std::string>& file_paths,
                                                        vk::Format                      format,
                                                        vk::ImageUsageFlags             usage_flags,
                                                        vk::ImageAspectFlags            aspect_mask,
                                                        vk::FormatFeatureFlags          format_feature_flags,
                                                        bool                            anisotropy_enable,
                                                        bool                            force_staging)
    {
        auto [width, height] = g_runtime_context.file_system->GetImageFileWidthHeight(file_paths[0]);
        if (width == 0 || height == 0)
        {
            return nullptr;
        }
        vk::Extent2D extent = {width, height};

        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        auto image_data_ptr = std::make_shared<ImageData>(nullptr);

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
        image_data_ptr->sampler = vk::raii::Sampler(logical_device, sampler_create_info);

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
            image_data_ptr->staging_buffer_data =
                BufferData(physical_device,
                           logical_device,
                           extent.width * extent.height * 4 * 6, // cubemap have 6 images
                           vk::BufferUsageFlagBits::eTransferSrc);
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

        vk::ImageCreateInfo image_create_info(vk::ImageCreateFlagBits::eCubeCompatible,
                                              vk::ImageType::e2D,
                                              format,
                                              vk::Extent3D(extent, 1),
                                              1,
                                              6, // cubemap have 6 images
                                              vk::SampleCountFlagBits::e1,
                                              image_tiling,
                                              usage_flags | vk::ImageUsageFlagBits::eSampled,
                                              vk::SharingMode::eExclusive,
                                              {},
                                              initial_layout);
        image_data_ptr->image = vk::raii::Image(logical_device, image_create_info);

        image_data_ptr->device_memory = AllocateDeviceMemory(logical_device,
                                                             physical_device.getMemoryProperties(),
                                                             image_data_ptr->image.getMemoryRequirements(),
                                                             requirements);
        image_data_ptr->image.bindMemory(*image_data_ptr->device_memory, 0);
        image_data_ptr->image_view =
            vk::raii::ImageView(logical_device,
                                vk::ImageViewCreateInfo({},
                                                        *image_data_ptr->image,
                                                        vk::ImageViewType::e2D,
                                                        format,
                                                        {},
                                                        {aspect_mask, 0, 1, 0, 6})); // cubemap have 6 images

        // Read image from file to device memory

        void* data = image_data_ptr->need_staging ?
                         image_data_ptr->staging_buffer_data.device_memory.mapMemory(
                             0, image_data_ptr->staging_buffer_data.buffer.getMemoryRequirements().size) :
                         image_data_ptr->device_memory.mapMemory(0, image_data_ptr->image.getMemoryRequirements().size);

        // cubemap have 6 images
        for (std::size_t i = 0; i < 6; ++i)
        {
            if (g_runtime_context.file_system->ReadImageFileToPtr(
                    file_paths[i], static_cast<uint8_t*>(data) + extent.width * extent.height * 4 * i) == 0)
                return nullptr;
        }

        image_data_ptr->need_staging ? image_data_ptr->staging_buffer_data.device_memory.unmapMemory() :
                                       image_data_ptr->device_memory.unmapMemory();

        // Transit Layout

        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](const vk::raii::CommandBuffer& command_buffer) {
                          if (image_data_ptr->need_staging)
                          {
                              // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
                              image_data_ptr->SetLayout(
                                  command_buffer, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
                              std::vector<vk::BufferImageCopy> copy_regions;
                              // cubemap have 6 images
                              for (std::size_t i = 0; i < 6; ++i)
                                  copy_regions.emplace_back(extent.width * extent.height * 4 * i, /* bufferOffset */
                                                            image_data_ptr->extent.width,
                                                            image_data_ptr->extent.height,
                                                            vk::ImageSubresourceLayers(aspect_mask, 0, i, 1),
                                                            vk::Offset3D(0, 0, 0),
                                                            vk::Extent3D(image_data_ptr->extent, 1));
                              command_buffer.copyBufferToImage(*image_data_ptr->staging_buffer_data.buffer,
                                                               *image_data_ptr->image,
                                                               vk::ImageLayout::eTransferDstOptimal,
                                                               copy_regions);
                              // Set the layout for the texture image from eTransferDstOptimal to eShaderReadOnlyOptimal
                              image_data_ptr->SetLayout(command_buffer,
                                                        vk::ImageLayout::eTransferDstOptimal,
                                                        vk::ImageLayout::eShaderReadOnlyOptimal);
                          }
                          else
                          {
                              // If we can use the linear tiled image as a texture, just do it
                              image_data_ptr->SetLayout(command_buffer,
                                                        vk::ImageLayout::ePreinitialized,
                                                        vk::ImageLayout::eShaderReadOnlyOptimal);
                          }
                      });

        return image_data_ptr;
    }
} // namespace Meow
