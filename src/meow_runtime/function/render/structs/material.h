#pragma once

#include "core/base/non_copyable.h"
#include "function/render/render_resources/uniform_buffer.h"
#include "shader.h"

#include <memory>
#include <tuple>
#include <vector>

namespace Meow
{
    class Material : NonCopyable
    {
    public:
        Material(std::nullptr_t) {}

        Material(const vk::raii::PhysicalDevice& physical_device,
                 const vk::raii::Device&         logical_device,
                 std::shared_ptr<Shader>         shader_ptr);

        Material(Material&& rhs) noexcept
        {
            std::swap(shader_ptr, rhs.shader_ptr);
            this->color_attachment_count = rhs.color_attachment_count;
            this->subpass                = rhs.subpass;
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
                std::swap(graphics_pipeline, rhs.graphics_pipeline);
                this->actived   = rhs.actived;
                this->obj_count = rhs.obj_count;
                std::swap(per_obj_dynamic_offsets, rhs.per_obj_dynamic_offsets);
                std::swap(descriptor_sets, rhs.descriptor_sets);
            }

            return *this;
        }

        void CreatePipeline(const vk::raii::Device&     logical_device,
                            const vk::raii::RenderPass& render_pass,
                            vk::FrontFace               front_face,
                            bool                        depth_buffered);

        std::shared_ptr<Shader> GetShader() { return shader_ptr; }

        void BindPipeline(const vk::raii::CommandBuffer& command_buffer);

        void BeginPopulatingDynamicUniformBufferPerFrame();

        void EndPopulatingDynamicUniformBufferPerFrame();

        void BeginPopulatingDynamicUniformBufferPerObject();

        void EndPopulatingDynamicUniformBufferPerObject();

        void PopulateDynamicUniformBuffer(std::shared_ptr<UniformBuffer> buffer,
                                          const std::string&             name,
                                          void*                          dataPtr,
                                          uint32_t                       size);

        std::vector<uint32_t> GetDynamicOffsets(uint32_t obj_index);

        std::shared_ptr<Shader> shader_ptr             = nullptr;
        int                     color_attachment_count = 1;
        int                     subpass                = 0;

    private:
        vk::raii::Pipeline graphics_pipeline = nullptr;

        // stored for binding descriptor set

        bool                               actived   = false;
        int32_t                            obj_count = 0;
        std::vector<std::vector<uint32_t>> per_obj_dynamic_offsets;
        std::vector<vk::DescriptorSet>     descriptor_sets;
    };
} // namespace Meow
