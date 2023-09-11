#pragma once

#define NOMINMAX

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <iostream>

namespace vk
{
    namespace Meow
    {
        const uint64_t FenceTimeout = 100000000;

        template<typename TargetType, typename SourceType>
        VULKAN_HPP_INLINE TargetType checked_cast(SourceType value)
        {
            static_assert(sizeof(TargetType) <= sizeof(SourceType), "No need to cast from smaller to larger type!");
            static_assert(std::numeric_limits<SourceType>::is_integer, "Only integer types supported!");
            static_assert(!std::numeric_limits<SourceType>::is_signed, "Only unsigned types supported!");
            static_assert(std::numeric_limits<TargetType>::is_integer, "Only integer types supported!");
            static_assert(!std::numeric_limits<TargetType>::is_signed, "Only unsigned types supported!");
            assert(value <= std::numeric_limits<TargetType>::max());
            return static_cast<TargetType>(value);
        }

        std::vector<const char*>
        GetRequiredInstanceExtensions(std::vector<const char*> const& required_instance_extensions);

        bool ValidateExtensions(const std::vector<const char*>&             required,
                                const std::vector<vk::ExtensionProperties>& available);

        bool ValidateLayers(const std::vector<const char*>&         required,
                            const std::vector<vk::LayerProperties>& available);

        std::vector<const char*>
        GetOptimalValidationLayers(const std::vector<vk::LayerProperties>& supported_instance_layers);

        VKAPI_ATTR VkBool32 VKAPI_CALL
        DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                    VkDebugUtilsMessageTypeFlagsEXT             messageTypes,
                                    VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
                                    void* /*pUserData*/);

        vk::DebugUtilsMessengerCreateInfoEXT MakeDebugUtilsMessengerCreateInfoEXT();

        void LogVulkanAPIVersion(const uint32_t api_version);

        void LogVulkanDevice(const VkPhysicalDeviceProperties&         physical_device_properties,
                             const std::vector<VkExtensionProperties>& extension_properties);

        uint32_t ScorePhysicalDevice(const vk::raii::PhysicalDevice& device,
                                     const std::vector<const char*>& required_device_extensions);

        struct SurfaceData
        {
            SurfaceData(vk::raii::Instance const& instance, GLFWwindow* glfw_window, vk::Extent2D const& extent_) :
                extent(extent_)
            {
                VkSurfaceKHR _surface;
                VkResult     err =
                    glfwCreateWindowSurface(static_cast<VkInstance>(*instance), glfw_window, nullptr, &_surface);
                if (err != VK_SUCCESS)
                    throw std::runtime_error("Failed to create window!");
                surface = vk::raii::SurfaceKHR(instance, _surface);
            }

            vk::Extent2D         extent;
            vk::raii::SurfaceKHR surface = nullptr;
        };

        uint32_t FindGraphicsQueueFamilyIndex(std::vector<vk::QueueFamilyProperties> const& queue_family_properties);

        std::pair<uint32_t, uint32_t>
        FindGraphicsAndPresentQueueFamilyIndex(vk::raii::PhysicalDevice const& physical_device,
                                               vk::raii::SurfaceKHR const&     surface);

        vk::SurfaceFormatKHR PickSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& formats);

        vk::PresentModeKHR PickPresentMode(std::vector<vk::PresentModeKHR> const& present_modes);

        struct SwapChainData
        {
            SwapChainData(vk::raii::PhysicalDevice const& physical_device,
                          vk::raii::Device const&         device,
                          vk::raii::SurfaceKHR const&     surface,
                          vk::Extent2D const&             extent,
                          vk::ImageUsageFlags             usage,
                          vk::raii::SwapchainKHR const*   p_old_swapchain,
                          uint32_t                        graphics_queue_family_index,
                          uint32_t                        present_queue_family_index)
            {
                vk::SurfaceFormatKHR surface_format =
                    vk::Meow::PickSurfaceFormat(physical_device.getSurfaceFormatsKHR(*surface));
                color_format = surface_format.format;

                vk::SurfaceCapabilitiesKHR surface_capabilities = physical_device.getSurfaceCapabilitiesKHR(*surface);
                vk::Extent2D               swapchain_extent;
                if (surface_capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
                {
                    // If the surface size is undefined, the size is set to the size of the images requested.
                    swapchain_extent.width  = glm::clamp(extent.width,
                                                        surface_capabilities.minImageExtent.width,
                                                        surface_capabilities.maxImageExtent.width);
                    swapchain_extent.height = glm::clamp(extent.height,
                                                         surface_capabilities.minImageExtent.height,
                                                         surface_capabilities.maxImageExtent.height);
                }
                else
                {
                    // If the surface size is defined, the swap chain size must match
                    swapchain_extent = surface_capabilities.currentExtent;
                }
                vk::SurfaceTransformFlagBitsKHR pre_transform =
                    (surface_capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ?
                        vk::SurfaceTransformFlagBitsKHR::eIdentity :
                        surface_capabilities.currentTransform;
                vk::CompositeAlphaFlagBitsKHR composite_alpha =
                    (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ?
                        vk::CompositeAlphaFlagBitsKHR::ePreMultiplied :
                    (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ?
                        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied :
                    (surface_capabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ?
                        vk::CompositeAlphaFlagBitsKHR::eInherit :
                        vk::CompositeAlphaFlagBitsKHR::eOpaque;
                vk::PresentModeKHR present_mode =
                    vk::Meow::PickPresentMode(physical_device.getSurfacePresentModesKHR(*surface));
                vk::SwapchainCreateInfoKHR swap_chain_create_info(
                    {},
                    *surface,
                    glm::clamp(3u, surface_capabilities.minImageCount, surface_capabilities.maxImageCount),
                    color_format,
                    surface_format.colorSpace,
                    swapchain_extent,
                    1,
                    usage,
                    vk::SharingMode::eExclusive,
                    {},
                    pre_transform,
                    composite_alpha,
                    present_mode,
                    true,
                    p_old_swapchain ? **p_old_swapchain : nullptr);
                if (graphics_queue_family_index != present_queue_family_index)
                {
                    uint32_t queueFamilyIndices[2] = {graphics_queue_family_index, present_queue_family_index};
                    // If the graphics and present queues are from different queue families, we either have to
                    // explicitly transfer ownership of images between the queues, or we have to create the swapchain
                    // with imageSharingMode as vk::SharingMode::eConcurrent
                    swap_chain_create_info.imageSharingMode      = vk::SharingMode::eConcurrent;
                    swap_chain_create_info.queueFamilyIndexCount = 2;
                    swap_chain_create_info.pQueueFamilyIndices   = queueFamilyIndices;
                }
                swap_chain = vk::raii::SwapchainKHR(device, swap_chain_create_info);

                images = swap_chain.getImages();

                image_views.reserve(images.size());
                vk::ImageViewCreateInfo image_view_create_info(
                    {}, {}, vk::ImageViewType::e2D, color_format, {}, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
                for (auto image : images)
                {
                    image_view_create_info.image = image;
                    image_views.emplace_back(device, image_view_create_info);
                }
            }

            vk::Format                       color_format;
            vk::raii::SwapchainKHR           swap_chain = nullptr;
            std::vector<vk::Image>           images;
            std::vector<vk::raii::ImageView> image_views;
        };

        uint32_t FindMemoryType(vk::PhysicalDeviceMemoryProperties const& memory_properties,
                                uint32_t                                  type_bits,
                                vk::MemoryPropertyFlags                   requirements_mask);

        vk::raii::DeviceMemory AllocateDeviceMemory(vk::raii::Device const&                   device,
                                                    vk::PhysicalDeviceMemoryProperties const& memory_properties,
                                                    vk::MemoryRequirements const&             memory_requirements,
                                                    vk::MemoryPropertyFlags                   memory_property_flags);

        template<typename T>
        void CopyToDevice(vk::raii::DeviceMemory const& device_memory,
                          T const*                      p_data,
                          size_t                        count,
                          vk::DeviceSize                stride = sizeof(T))
        {
            assert(sizeof(T) <= stride);
            uint8_t* device_data = static_cast<uint8_t*>(device_memory.mapMemory(0, count * stride));
            if (stride == sizeof(T))
            {
                memcpy(device_data, p_data, count * sizeof(T));
            }
            else
            {
                for (size_t i = 0; i < count; i++)
                {
                    memcpy(device_data, &p_data[i], sizeof(T));
                    device_data += stride;
                }
            }
            device_memory.unmapMemory();
        }

        template<typename T>
        void CopyToDevice(vk::raii::DeviceMemory const& device_memory, T const& data)
        {
            CopyToDevice<T>(device_memory, &data, 1);
        }

        struct ImageData
        {
            ImageData(vk::raii::PhysicalDevice const& physical_device,
                      vk::raii::Device const&         device,
                      vk::Format                      format_,
                      vk::Extent2D const&             extent,
                      vk::ImageTiling                 tiling,
                      vk::ImageUsageFlags             usage,
                      vk::ImageLayout                 initial_layout,
                      vk::MemoryPropertyFlags         memory_properties,
                      vk::ImageAspectFlags            aspect_mask) :
                format(format_),
                image(device,
                      {vk::ImageCreateFlags(),
                       vk::ImageType::e2D,
                       format,
                       vk::Extent3D(extent, 1),
                       1,
                       1,
                       vk::SampleCountFlagBits::e1,
                       tiling,
                       usage | vk::ImageUsageFlagBits::eSampled,
                       vk::SharingMode::eExclusive,
                       {},
                       initial_layout})
            {
                device_memory = vk::Meow::AllocateDeviceMemory(
                    device, physical_device.getMemoryProperties(), image.getMemoryRequirements(), memory_properties);
                image.bindMemory(*device_memory, 0);
                image_view = vk::raii::ImageView(
                    device,
                    vk::ImageViewCreateInfo({}, *image, vk::ImageViewType::e2D, format, {}, {aspect_mask, 0, 1, 0, 1}));
            }

            ImageData(std::nullptr_t) {}

            // the DeviceMemory should be destroyed before the Image it is bound to; to get that order with the standard
            // destructor of the ImageData, the order of DeviceMemory and Image here matters
            vk::Format             format;
            vk::raii::DeviceMemory device_memory = nullptr;
            vk::raii::Image        image         = nullptr;
            vk::raii::ImageView    image_view    = nullptr;
        };

        struct DepthBufferData : public ImageData
        {
            DepthBufferData(vk::raii::PhysicalDevice const& physical_device,
                            vk::raii::Device const&         device,
                            vk::Format                      format,
                            vk::Extent2D const&             extent) :
                ImageData(physical_device,
                          device,
                          format,
                          extent,
                          vk::ImageTiling::eOptimal,
                          vk::ImageUsageFlagBits::eDepthStencilAttachment,
                          vk::ImageLayout::eUndefined,
                          vk::MemoryPropertyFlagBits::eDeviceLocal,
                          vk::ImageAspectFlagBits::eDepth)
            {}
        };

        template<typename Func>
        void OneTimeSubmit(vk::raii::Device const&      device,
                           vk::raii::CommandPool const& command_pool,
                           vk::raii::Queue const&       queue,
                           Func const&                  func)
        {
            vk::raii::CommandBuffer command_buffer = std::move(
                vk::raii::CommandBuffers(device, {*command_pool, vk::CommandBufferLevel::ePrimary, 1}).front());
            command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
            func(command_buffer);
            command_buffer.end();
            vk::SubmitInfo submit_info(nullptr, nullptr, *command_buffer);
            queue.submit(submit_info, nullptr);
            queue.waitIdle();
        }

        struct BufferData
        {
            BufferData(vk::raii::PhysicalDevice const& physical_device,
                       vk::raii::Device const&         device,
                       vk::DeviceSize                  size,
                       vk::BufferUsageFlags            usage,
                       vk::MemoryPropertyFlags         property_flags = vk::MemoryPropertyFlagBits::eHostVisible |
                                                                vk::MemoryPropertyFlagBits::eHostCoherent) :
                buffer(device, vk::BufferCreateInfo({}, size, usage))
#if defined(MEOW_DEBUG)
                ,
                m_size(size), m_usage(usage), m_property_flags(property_flags)
#endif
            {
                device_memory = vk::Meow::AllocateDeviceMemory(
                    device, physical_device.getMemoryProperties(), buffer.getMemoryRequirements(), property_flags);
                buffer.bindMemory(*device_memory, 0);
            }

            BufferData(std::nullptr_t) {}

            template<typename DataType>
            void Upload(DataType const& data) const
            {
                assert((m_property_flags & vk::MemoryPropertyFlagBits::eHostCoherent) &&
                       (m_property_flags & vk::MemoryPropertyFlagBits::eHostVisible));
                assert(sizeof(DataType) <= m_size);

                void* data_ptr = device_memory.mapMemory(0, sizeof(DataType));
                memcpy(data_ptr, &data, sizeof(DataType));
                device_memory.unmapMemory();
            }

            template<typename DataType>
            void Upload(std::vector<DataType> const& data, size_t stride = 0) const
            {
                assert(m_property_flags & vk::MemoryPropertyFlagBits::eHostVisible);

                size_t element_size = stride ? stride : sizeof(DataType);
                assert(sizeof(DataType) <= element_size);

                vk::Meow::CopyToDevice(device_memory, data.data(), data.size(), element_size);
            }

            template<typename DataType>
            void Upload(vk::raii::PhysicalDevice const& physical_device,
                        vk::raii::Device const&         device,
                        vk::raii::CommandPool const&    command_pool,
                        vk::raii::Queue const&          queue,
                        std::vector<DataType> const&    data,
                        size_t                          stride) const
            {
                assert(m_usage & vk::BufferUsageFlagBits::eTransferDst);
                assert(m_property_flags & vk::MemoryPropertyFlagBits::eDeviceLocal);

                size_t element_size = stride ? stride : sizeof(DataType);
                assert(sizeof(DataType) <= element_size);

                size_t dataSize = data.size() * element_size;
                assert(dataSize <= m_size);

                vk::Meow::BufferData staging_buffer(
                    physical_device, device, dataSize, vk::BufferUsageFlagBits::eTransferSrc);
                vk::Meow::CopyToDevice(staging_buffer.device_memory, data.data(), data.size(), element_size);

                vk::Meow::OneTimeSubmit(
                    device, command_pool, queue, [&](vk::raii::CommandBuffer const& command_buffer) {
                        command_buffer.copyBuffer(
                            *staging_buffer.buffer, *this->buffer, vk::BufferCopy(0, 0, dataSize));
                    });
            }

            // the DeviceMemory should be destroyed before the Buffer it is bound to; to get that order with the
            // standard destructor of the BufferData, the order of DeviceMemory and Buffer here matters
            vk::raii::DeviceMemory device_memory = nullptr;
            vk::raii::Buffer       buffer        = nullptr;
#if defined(MEOW_DEBUG)
        private:
            vk::DeviceSize          m_size;
            vk::BufferUsageFlags    m_usage;
            vk::MemoryPropertyFlags m_property_flags;
#endif
        };

        void SetImageLayout(vk::raii::CommandBuffer const& command_buffer,
                            vk::Image                      image,
                            vk::Format                     format,
                            vk::ImageLayout                old_image_layout,
                            vk::ImageLayout                new_image_layout);

        struct TextureData
        {
            TextureData(vk::raii::PhysicalDevice const& physical_device,
                        vk::raii::Device const&         device,
                        vk::Extent2D const&             extent_              = {256, 256},
                        vk::ImageUsageFlags             usage_flags          = {},
                        vk::FormatFeatureFlags          format_feature_flags = {},
                        bool                            anisotropy_enable    = false,
                        bool                            force_staging        = false) :
                format(vk::Format::eR8G8B8A8Unorm),
                extent(extent_), sampler(device,
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
                need_staging = force_staging || ((format_properties.linearTilingFeatures & format_feature_flags) !=
                                                 format_feature_flags);
                vk::ImageTiling         image_tiling;
                vk::ImageLayout         initial_layout;
                vk::MemoryPropertyFlags requirements;
                if (need_staging)
                {
                    assert((format_properties.optimalTilingFeatures & format_feature_flags) == format_feature_flags);
                    staging_buffer_data = BufferData(physical_device,
                                                     device,
                                                     extent.width * extent.height * 4,
                                                     vk::BufferUsageFlagBits::eTransferSrc);
                    image_tiling        = vk::ImageTiling::eOptimal;
                    usage_flags |= vk::ImageUsageFlagBits::eTransferDst;
                    initial_layout = vk::ImageLayout::eUndefined;
                }
                else
                {
                    image_tiling   = vk::ImageTiling::eLinear;
                    initial_layout = vk::ImageLayout::ePreinitialized;
                    requirements = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
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

            template<typename ImageGenerator>
            void SetImage(vk::raii::CommandBuffer const& commandBuffer, ImageGenerator const& imageGenerator)
            {
                void* data = need_staging ?
                                 staging_buffer_data.device_memory.mapMemory(
                                     0, staging_buffer_data.buffer.getMemoryRequirements().size) :
                                 image_data.device_memory.mapMemory(0, image_data.image.getMemoryRequirements().size);
                imageGenerator(data, extent);
                need_staging ? staging_buffer_data.device_memory.unmapMemory() : image_data.device_memory.unmapMemory();

                if (need_staging)
                {
                    // Since we're going to blit to the texture image, set its layout to eTransferDstOptimal
                    vk::Meow::SetImageLayout(commandBuffer,
                                             *image_data.image,
                                             image_data.format,
                                             vk::ImageLayout::eUndefined,
                                             vk::ImageLayout::eTransferDstOptimal);
                    vk::BufferImageCopy copyRegion(0,
                                                   extent.width,
                                                   extent.height,
                                                   vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
                                                   vk::Offset3D(0, 0, 0),
                                                   vk::Extent3D(extent, 1));
                    commandBuffer.copyBufferToImage(*staging_buffer_data.buffer,
                                                    *image_data.image,
                                                    vk::ImageLayout::eTransferDstOptimal,
                                                    copyRegion);
                    // Set the layout for the texture image from eTransferDstOptimal to eShaderReadOnlyOptimal
                    vk::Meow::SetImageLayout(commandBuffer,
                                             *image_data.image,
                                             image_data.format,
                                             vk::ImageLayout::eTransferDstOptimal,
                                             vk::ImageLayout::eShaderReadOnlyOptimal);
                }
                else
                {
                    // If we can use the linear tiled image as a texture, just do it
                    vk::Meow::SetImageLayout(commandBuffer,
                                             *image_data.image,
                                             image_data.format,
                                             vk::ImageLayout::ePreinitialized,
                                             vk::ImageLayout::eShaderReadOnlyOptimal);
                }
            }

            vk::Format        format;
            vk::Extent2D      extent;
            bool              need_staging;
            BufferData        staging_buffer_data = nullptr;
            ImageData         image_data          = nullptr;
            vk::raii::Sampler sampler;
        };

        vk::raii::DescriptorSetLayout MakeDescriptorSetLayout(
            vk::raii::Device const&                                                            device,
            std::vector<std::tuple<vk::DescriptorType, uint32_t, vk::ShaderStageFlags>> const& binding_data,
            vk::DescriptorSetLayoutCreateFlags                                                 flags = {});

        void UpdateDescriptorSets(vk::raii::Device const&                                     device,
                                  vk::raii::DescriptorSet const&                              descriptor_set,
                                  std::vector<std::tuple<vk::DescriptorType,
                                                         vk::raii::Buffer const&,
                                                         vk::DeviceSize,
                                                         vk::raii::BufferView const*>> const& buffer_data,
                                  vk::Meow::TextureData const&                                texture_data,
                                  uint32_t                                                    binding_offset = 0);

        void UpdateDescriptorSets(vk::raii::Device const&                                     device,
                                  vk::raii::DescriptorSet const&                              descriptor_set,
                                  std::vector<std::tuple<vk::DescriptorType,
                                                         vk::raii::Buffer const&,
                                                         vk::DeviceSize,
                                                         vk::raii::BufferView const*>> const& buffer_data,
                                  std::vector<vk::Meow::TextureData> const&                   texture_data,
                                  uint32_t                                                    binding_offset = 0);

        vk::raii::RenderPass MakeRenderPass(vk::raii::Device const& device,
                                            vk::Format              color_format,
                                            vk::Format              depth_format,
                                            vk::AttachmentLoadOp    load_op    = vk::AttachmentLoadOp::eClear,
                                            vk::ImageLayout color_final_layout = vk::ImageLayout::ePresentSrcKHR);

        std::vector<vk::raii::Framebuffer> MakeFramebuffers(vk::raii::Device const&                 device,
                                                            vk::raii::RenderPass&                   render_pass,
                                                            std::vector<vk::raii::ImageView> const& image_views,
                                                            vk::raii::ImageView const*              p_depth_image_view,
                                                            vk::Extent2D const&                     extent);

        vk::raii::Pipeline
        MakeGraphicsPipeline(vk::raii::Device const&                             device,
                             vk::raii::PipelineCache const&                      pipeline_cache,
                             vk::raii::ShaderModule const&                       vertex_shader_module,
                             vk::SpecializationInfo const*                       vertex_shader_specialization_info,
                             vk::raii::ShaderModule const&                       fragment_shader_module,
                             vk::SpecializationInfo const*                       fragment_shader_specialization_info,
                             uint32_t                                            vertex_stride,
                             std::vector<std::pair<vk::Format, uint32_t>> const& vertex_input_attribute_format_offset,
                             vk::FrontFace                                       front_face,
                             bool                                                depth_buffered,
                             vk::raii::PipelineLayout const&                     pipeline_layout,
                             vk::raii::RenderPass const&                         render_pass);
    } // namespace Meow
} // namespace vk