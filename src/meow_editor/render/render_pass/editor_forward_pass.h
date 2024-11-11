#pragma once

#include "meow_runtime/function/render/render_pass/render_pass.h"
#include "meow_runtime/function/render/structs/material.h"
#include "meow_runtime/function/render/structs/shader.h"
#include "render/structs/builtin_render_stat.h"

namespace Meow
{
    class EditorForwardPass : public RenderPass
    {
    public:
        EditorForwardPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        EditorForwardPass(const vk::raii::PhysicalDevice& physical_device,
                          const vk::raii::Device&         device,
                          SurfaceData&                    surface_data,
                          const vk::raii::CommandPool&    command_pool,
                          const vk::raii::Queue&          queue,
                          DescriptorAllocatorGrowable&    m_descriptor_allocator);

        EditorForwardPass(EditorForwardPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        EditorForwardPass& operator=(EditorForwardPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~EditorForwardPass() override
        {
            m_forward_mat = nullptr;
            render_pass   = nullptr;
            framebuffers.clear();
            m_depth_attachment = nullptr;
        }

        void RefreshFrameBuffers(const vk::raii::PhysicalDevice&   physical_device,
                                 const vk::raii::Device&           device,
                                 const vk::raii::CommandPool&      command_pool,
                                 const vk::raii::Queue&            queue,
                                 const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        void AfterPresent() override;

        friend void swap(EditorForwardPass& lhs, EditorForwardPass& rhs);

    private:
        Material m_forward_mat = nullptr;

        int draw_call = 0;

        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;

        BuiltinRenderStat m_render_stat;
    };
} // namespace Meow
