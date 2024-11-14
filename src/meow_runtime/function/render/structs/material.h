#pragma once

#include "buffer_data.h"
#include "core/base/non_copyable.h"
#include "ring_uniform_buffer.h"
#include "shader.h"

#include <limits>
#include <memory>
#include <tuple>
#include <vector>

namespace Meow
{
    class Material : NonCopyable
    {
    public:
        Material(std::nullptr_t) {}

        Material(vk::raii::PhysicalDevice const& physical_device,
                 vk::raii::Device const&         logical_device,
                 std::shared_ptr<Shader>         shader_ptr);

        Material(Material&& rhs) noexcept
        {
            std::swap(shader_ptr, rhs.shader_ptr);
            this->color_attachment_count = rhs.color_attachment_count;
            this->subpass                = rhs.subpass;
            std::swap(ring_buffer, rhs.ring_buffer);
            std::swap(graphics_pipeline, rhs.graphics_pipeline);
            this->actived   = rhs.actived;
            this->obj_count = rhs.obj_count;
            std::swap(per_obj_dynamic_offsets, rhs.per_obj_dynamic_offsets);
            std::swap(descriptor_sets, rhs.descriptor_sets);
        }

        Material& operator=(Material&& rhs) noexcept
        {
            if (this != &rhs)
            {
                std::swap(shader_ptr, rhs.shader_ptr);
                this->color_attachment_count = rhs.color_attachment_count;
                this->subpass                = rhs.subpass;
                std::swap(ring_buffer, rhs.ring_buffer);
                std::swap(graphics_pipeline, rhs.graphics_pipeline);
                this->actived   = rhs.actived;
                this->obj_count = rhs.obj_count;
                std::swap(per_obj_dynamic_offsets, rhs.per_obj_dynamic_offsets);
                std::swap(descriptor_sets, rhs.descriptor_sets);
            }

            return *this;
        }

        void CreatePipeline(vk::raii::Device const&     logical_device,
                            vk::raii::RenderPass const& render_pass,
                            vk::FrontFace               front_face,
                            bool                        depth_buffered);

        void BeginPopulatingDynamicUniformBufferPerFrame();

        void EndPopulatingDynamicUniformBufferPerFrame();

        void BeginPopulatingDynamicUniformBufferPerObject();

        void EndPopulatingDynamicUniformBufferPerObject();

        void PopulateDynamicUniformBuffer(const std::string& name, void* dataPtr, uint32_t size);

        void SetStorageBuffer(vk::raii::Device const&     logical_device,
                              const std::string&          name,
                              vk::raii::Buffer const&     buffer,
                              vk::DeviceSize              range            = VK_WHOLE_SIZE,
                              vk::raii::BufferView const* raii_buffer_view = nullptr);

        void SetImage(vk::raii::Device const& logical_device, const std::string& name, ImageData& image_data);

        void BindPipeline(vk::raii::CommandBuffer const& command_buffer);

        void BindAllDescriptorSets(vk::raii::CommandBuffer const& command_buffer, int32_t obj_index);

        RingUniformBuffer const& GetRingUniformBuffer() const { return ring_buffer; }

    public:
        std::shared_ptr<Shader> shader_ptr             = nullptr;
        int                     color_attachment_count = 1;
        int                     subpass                = 0;

    private:
        RingUniformBuffer ring_buffer = nullptr;

        vk::raii::Pipeline graphics_pipeline = nullptr;

        // stored for binding descriptor set

        bool                               actived   = false;
        int32_t                            obj_count = 0;
        std::vector<std::vector<uint32_t>> per_obj_dynamic_offsets;
        std::vector<vk::DescriptorSet>     descriptor_sets;
    };

} // namespace Meow
