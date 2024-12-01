#include "image_data.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    ImageData ImageData::CreateTexture(const std::string&     file_path,
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

        ImageData image_data = nullptr;

        // Create Texture

        image_data.format      = format;
        image_data.extent      = extent;
        image_data.size        = extent.width * extent.height * 4;
        image_data.aspect_mask = aspect_mask;

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
        image_data.sampler = vk::raii::Sampler(logical_device, sampler_create_info);

        vk::FormatProperties format_properties = physical_device.getFormatProperties(format);

        format_feature_flags |= vk::FormatFeatureFlagBits::eSampledImage;
        image_data.need_staging =
            force_staging || ((format_properties.linearTilingFeatures & format_feature_flags) != format_feature_flags);
        vk::ImageTiling         image_tiling;
        vk::ImageLayout         initial_layout;
        vk::MemoryPropertyFlags requirements;
        if (image_data.need_staging)
        {
            assert((format_properties.optimalTilingFeatures & format_feature_flags) == format_feature_flags);
            image_data.staging_buffer_data =
                BufferData(physical_device, logical_device, image_data.size, vk::BufferUsageFlagBits::eTransferSrc);
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
        image_data.image = vk::raii::Image(logical_device, image_create_info);

        image_data.device_memory = AllocateDeviceMemory(logical_device,
                                                        physical_device.getMemoryProperties(),
                                                        image_data.image.getMemoryRequirements(),
                                                        requirements);
        image_data.image.bindMemory(*image_data.device_memory, 0);
        image_data.image_view = vk::raii::ImageView(
            logical_device,
            vk::ImageViewCreateInfo(
                {}, *image_data.image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        // Read image from file to device memory

        void* data = image_data.need_staging ?
                         image_data.staging_buffer_data.device_memory.mapMemory(
                             0, image_data.staging_buffer_data.buffer.getMemoryRequirements().size) :
                         image_data.device_memory.mapMemory(0, image_data.image.getMemoryRequirements().size);

        if (g_runtime_context.file_system->ReadImageFileToPtr(file_path, static_cast<uint8_t*>(data)) == 0)
            return nullptr;

        image_data.need_staging ? image_data.staging_buffer_data.device_memory.unmapMemory() :
                                  image_data.device_memory.unmapMemory();

        // Transit Layout

        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](const vk::raii::CommandBuffer& command_buffer) {
                          if (image_data.need_staging)
                          {
                              // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
                              TransitLayout(command_buffer,
                                            *image_data.image,
                                            vk::ImageLayout::eUndefined,
                                            vk::ImageLayout::eTransferDstOptimal,
                                            {aspect_mask, 0, 1, 0, 1});
                              vk::BufferImageCopy copy_region(0,
                                                              image_data.extent.width,
                                                              image_data.extent.height,
                                                              vk::ImageSubresourceLayers(aspect_mask, 0, 0, 1),
                                                              vk::Offset3D(0, 0, 0),
                                                              vk::Extent3D(image_data.extent, 1));
                              command_buffer.copyBufferToImage(*image_data.staging_buffer_data.buffer,
                                                               *image_data.image,
                                                               vk::ImageLayout::eTransferDstOptimal,
                                                               copy_region);
                              // Set the layout for the texture image from eTransferDstOptimal to eShaderReadOnlyOptimal
                              TransitLayout(command_buffer,
                                            *image_data.image,
                                            vk::ImageLayout::eTransferDstOptimal,
                                            vk::ImageLayout::eShaderReadOnlyOptimal,
                                            {aspect_mask, 0, 1, 0, 1});
                          }
                          else
                          {
                              // If we can use the linear tiled image as a texture, just do it
                              TransitLayout(command_buffer,
                                            *image_data.image,
                                            vk::ImageLayout::ePreinitialized,
                                            vk::ImageLayout::eShaderReadOnlyOptimal,
                                            {aspect_mask, 0, 1, 0, 1});
                          }
                      });

        return image_data;
    }

    ImageData ImageData::CreateAttachment(vk::Format             format,
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

        ImageData image_data = nullptr;

        // Create Attachment

        auto                    image_tiling   = vk::ImageTiling::eOptimal;
        auto                    initial_layout = vk::ImageLayout::eUndefined;
        vk::MemoryPropertyFlags requirements   = vk::MemoryPropertyFlagBits::eDeviceLocal;

        image_data.format      = format;
        image_data.extent      = extent;
        image_data.size        = extent.width * extent.height * 4;
        image_data.aspect_mask = aspect_mask;

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
        image_data.image = vk::raii::Image(logical_device, image_create_info);

        image_data.device_memory = AllocateDeviceMemory(logical_device,
                                                        physical_device.getMemoryProperties(),
                                                        image_data.image.getMemoryRequirements(),
                                                        requirements);
        image_data.image.bindMemory(*image_data.device_memory, 0);
        image_data.image_view = vk::raii::ImageView(
            logical_device,
            vk::ImageViewCreateInfo(
                {}, *image_data.image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        // Transit Layout
        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](const vk::raii::CommandBuffer& command_buffer) {
                          if (aspect_mask & vk::ImageAspectFlagBits::eColor)
                              TransitLayout(command_buffer,
                                            *image_data.image,
                                            vk::ImageLayout::eUndefined,
                                            vk::ImageLayout::eColorAttachmentOptimal,
                                            {aspect_mask, 0, 1, 0, 1});
                          else if (aspect_mask & vk::ImageAspectFlagBits::eDepth)
                              TransitLayout(command_buffer,
                                            *image_data.image,
                                            vk::ImageLayout::eUndefined,
                                            vk::ImageLayout::eDepthStencilAttachmentOptimal,
                                            {aspect_mask, 0, 1, 0, 1});
                      });

        return image_data;
    }

    ImageData ImageData::CreateRenderTarget(vk::Format             format,
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

        ImageData image_data = nullptr;

        // Create Attachment

        auto                    image_tiling   = vk::ImageTiling::eOptimal;
        auto                    initial_layout = vk::ImageLayout::eUndefined;
        vk::MemoryPropertyFlags requirements   = vk::MemoryPropertyFlagBits::eDeviceLocal;

        image_data.format      = format;
        image_data.extent      = extent;
        image_data.size        = extent.width * extent.height * 4;
        image_data.aspect_mask = aspect_mask;

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
        image_data.sampler = vk::raii::Sampler(logical_device, sampler_create_info);

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
        image_data.image = vk::raii::Image(logical_device, image_create_info);

        image_data.device_memory = AllocateDeviceMemory(logical_device,
                                                        physical_device.getMemoryProperties(),
                                                        image_data.image.getMemoryRequirements(),
                                                        requirements);
        image_data.image.bindMemory(*image_data.device_memory, 0);
        image_data.image_view = vk::raii::ImageView(
            logical_device,
            vk::ImageViewCreateInfo(
                {}, *image_data.image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));

        // Transit Layout
        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](const vk::raii::CommandBuffer& command_buffer) {
                          TransitLayout(command_buffer,
                                        *image_data.image,
                                        vk::ImageLayout::eUndefined,
                                        vk::ImageLayout::eShaderReadOnlyOptimal,
                                        {aspect_mask, 0, 1, 0, 1});
                      });

        return image_data;
    }

    ImageData ImageData::CreateCubemap(const std::vector<std::string>& file_paths,
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

        ImageData image_data = nullptr;

        // Create Texture

        image_data.format      = format;
        image_data.extent      = extent;
        image_data.size        = extent.width * extent.height * 4 * 4;
        image_data.aspect_mask = aspect_mask;

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
        image_data.sampler = vk::raii::Sampler(logical_device, sampler_create_info);

        vk::FormatProperties format_properties = physical_device.getFormatProperties(format);

        format_feature_flags |= vk::FormatFeatureFlagBits::eSampledImage;
        image_data.need_staging =
            force_staging || ((format_properties.linearTilingFeatures & format_feature_flags) != format_feature_flags);
        vk::ImageTiling         image_tiling;
        vk::ImageLayout         initial_layout;
        vk::MemoryPropertyFlags requirements;
        if (image_data.need_staging)
        {
            assert((format_properties.optimalTilingFeatures & format_feature_flags) == format_feature_flags);
            image_data.staging_buffer_data = BufferData(physical_device,
                                                        logical_device,
                                                        image_data.size * 6, // cubemap have 6 images
                                                        vk::BufferUsageFlagBits::eTransferSrc);
            image_tiling                   = vk::ImageTiling::eOptimal;
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
        image_data.image = vk::raii::Image(logical_device, image_create_info);

        image_data.device_memory = AllocateDeviceMemory(logical_device,
                                                        physical_device.getMemoryProperties(),
                                                        image_data.image.getMemoryRequirements(),
                                                        requirements);
        image_data.image.bindMemory(*image_data.device_memory, 0);
        image_data.image_view =
            vk::raii::ImageView(logical_device,
                                vk::ImageViewCreateInfo({},
                                                        *image_data.image,
                                                        vk::ImageViewType::eCube,
                                                        format,
                                                        {},
                                                        {aspect_mask, 0, 1, 0, 6})); // cubemap have 6 images

        // Read image from file to device memory

        void* data = image_data.need_staging ?
                         image_data.staging_buffer_data.device_memory.mapMemory(
                             0, image_data.staging_buffer_data.buffer.getMemoryRequirements().size) :
                         image_data.device_memory.mapMemory(0, image_data.image.getMemoryRequirements().size);

        // cubemap have 6 images
        for (std::size_t i = 0; i < 6; ++i)
        {
            if (g_runtime_context.file_system->ReadImageFileToPtr(
                    file_paths[i], static_cast<uint8_t*>(data) + image_data.size * i) == 0)
                return nullptr;
        }

        image_data.need_staging ? image_data.staging_buffer_data.device_memory.unmapMemory() :
                                  image_data.device_memory.unmapMemory();

        // Transit Layout

        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [&](const vk::raii::CommandBuffer& command_buffer) {
                          if (image_data.need_staging)
                          {
                              // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
                              TransitLayout(command_buffer,
                                            *image_data.image,
                                            vk::ImageLayout::eUndefined,
                                            vk::ImageLayout::eTransferDstOptimal,
                                            {aspect_mask, 0, 1, 0, 6});
                              std::vector<vk::BufferImageCopy> copy_regions;
                              // cubemap have 6 images
                              for (std::size_t i = 0; i < 6; ++i)
                                  copy_regions.emplace_back(image_data.size * i, /* bufferOffset */
                                                            image_data.extent.width,
                                                            image_data.extent.height,
                                                            vk::ImageSubresourceLayers(aspect_mask, 0, i, 1),
                                                            vk::Offset3D(0, 0, 0),
                                                            vk::Extent3D(image_data.extent, 1));
                              command_buffer.copyBufferToImage(*image_data.staging_buffer_data.buffer,
                                                               *image_data.image,
                                                               vk::ImageLayout::eTransferDstOptimal,
                                                               copy_regions);
                              // Set the layout for the texture image from eTransferDstOptimal to eShaderReadOnlyOptimal
                              TransitLayout(command_buffer,
                                            *image_data.image,
                                            vk::ImageLayout::eTransferDstOptimal,
                                            vk::ImageLayout::eShaderReadOnlyOptimal,
                                            {aspect_mask, 0, 1, 0, 6});
                          }
                          else
                          {
                              // If we can use the linear tiled image as a texture, just do it
                              TransitLayout(command_buffer,
                                            *image_data.image,
                                            vk::ImageLayout::ePreinitialized,
                                            vk::ImageLayout::eShaderReadOnlyOptimal,
                                            {aspect_mask, 0, 1, 0, 6});
                          }
                      });

        return image_data;
    }
} // namespace Meow
