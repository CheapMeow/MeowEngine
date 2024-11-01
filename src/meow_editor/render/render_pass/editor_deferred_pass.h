#pragma once

#include "meow_runtime/function/render/render_pass/render_pass.h"
#include "meow_runtime/function/render/structs/material.h"
#include "meow_runtime/function/render/structs/model.h"
#include "meow_runtime/function/render/structs/shader.h"
#include "render/structs/builtin_render_stat.h"

namespace Meow
{
    const int k_num_lights = 64;

    struct PointLight
    {
        glm::vec4 position;
        glm::vec3 color;
        float     radius;
    };

    struct LightSpawnBlock
    {
        glm::vec3 position[k_num_lights];
        glm::vec3 direction[k_num_lights];
        float     speed[k_num_lights];
    };

    struct LightDataBlock
    {
        PointLight lights[k_num_lights];
    };

    class EditorDeferredPass : public RenderPass
    {
    public:
        EditorDeferredPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        EditorDeferredPass(const vk::raii::PhysicalDevice& physical_device,
                     const vk::raii::Device&         device,
                     SurfaceData&                    surface_data,
                     const vk::raii::CommandPool&    command_pool,
                     const vk::raii::Queue&          queue,
                     DescriptorAllocatorGrowable&    m_descriptor_allocator);

        EditorDeferredPass(EditorDeferredPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        EditorDeferredPass& operator=(EditorDeferredPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~EditorDeferredPass() override
        {
            m_obj2attachment_mat = nullptr;
            m_quad_mat           = nullptr;
            m_quad_model         = nullptr;
            render_pass          = nullptr;
            framebuffers.clear();
            m_color_attachment  = nullptr;
            m_normal_attachment = nullptr;
            m_depth_attachment  = nullptr;
        }

        void RefreshFrameBuffers(const vk::raii::PhysicalDevice&         physical_device,
                                 const vk::raii::Device&                 device,
                                 const vk::raii::CommandPool&            command_pool,
                                 const vk::raii::Queue&                  queue,
                                 SurfaceData&                            surface_data,
                                 const std::vector<vk::raii::ImageView>& swapchain_image_views,
                                 const vk::Extent2D&                     extent) override;

        void UpdateUniformBuffer() override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   const Meow::SurfaceData&       surface_data,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        void AfterPresent() override;

        friend void swap(EditorDeferredPass& lhs, EditorDeferredPass& rhs);

    private:
        Material m_obj2attachment_mat = nullptr;
        Material m_quad_mat           = nullptr;
        Model    m_quad_model         = nullptr;

        std::shared_ptr<ImageData> m_color_attachment    = nullptr;
        std::shared_ptr<ImageData> m_normal_attachment   = nullptr;
        std::shared_ptr<ImageData> m_position_attachment = nullptr;

        LightDataBlock  m_LightDatas;
        LightSpawnBlock m_LightInfos;

        int draw_call = 0;

        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;
        
        std::string       m_pass_names[2];
        BuiltinRenderStat m_render_stat[2];
    };
} // namespace Meow
