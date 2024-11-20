#pragma once

#include "function/render/render_pass/render_pass.h"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"

namespace Meow
{
    class ForwardPass : public RenderPass
    {
    public:
        ForwardPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        ForwardPass(const vk::raii::Device& logical_device)
            : RenderPass(logical_device)
        {}

        ForwardPass(ForwardPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        ForwardPass& operator=(ForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~ForwardPass() override = default;

        void CreateMaterial(const vk::raii::PhysicalDevice& physical_device,
                            const vk::raii::Device&         logical_device,
                            const vk::raii::CommandPool&    command_pool,
                            const vk::raii::Queue&          queue,
                            DescriptorAllocatorGrowable&    descriptor_allocator);

        void RefreshFrameBuffers(const vk::raii::PhysicalDevice&   physical_device,
                                 const vk::raii::Device&           logical_device,
                                 const vk::raii::CommandPool&      command_pool,
                                 const vk::raii::Queue&            queue,
                                 const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void DrawOnly(const vk::raii::CommandBuffer& command_buffer);

        friend void swap(ForwardPass& lhs, ForwardPass& rhs);

    protected:
        Material m_forward_mat = nullptr;

        vk::raii::DescriptorSets m_forward_descriptor_sets = nullptr;

        std::shared_ptr<UniformBuffer> m_per_scene_uniform_buffer;
        std::shared_ptr<UniformBuffer> m_dynamic_uniform_buffer;

        int draw_call = 0;
    };
} // namespace Meow