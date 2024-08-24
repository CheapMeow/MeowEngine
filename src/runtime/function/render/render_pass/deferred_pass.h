#pragma once

#include "function/render/structs/builtin_render_stat.h"
#include "function/render/structs/material.h"
#include "function/render/structs/model.h"
#include "function/render/structs/shader.h"
#include "render_pass.h"


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

    class DeferredPass : public RenderPass
    {
    public:
        DeferredPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

        DeferredPass(vk::raii::PhysicalDevice const& physical_device,
                     vk::raii::Device const&         device,
                     SurfaceData&                    surface_data,
                     vk::raii::CommandPool const&    command_pool,
                     vk::raii::Queue const&          queue,
                     DescriptorAllocatorGrowable&    m_descriptor_allocator);

        DeferredPass(DeferredPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        DeferredPass& operator=(DeferredPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~DeferredPass()
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

        void RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                 vk::raii::Device const&                 device,
                                 vk::raii::CommandBuffer const&          command_buffer,
                                 SurfaceData&                            surface_data,
                                 std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                 vk::Extent2D const&                     extent) override;

        void UpdateUniformBuffer() override;

        void Start(vk::raii::CommandBuffer const& command_buffer,
                   Meow::SurfaceData const&       surface_data,
                   uint32_t                       current_image_index) override;

        void Draw(vk::raii::CommandBuffer const& command_buffer) override;

        void AfterPresent() override;

        friend void swap(DeferredPass& lhs, DeferredPass& rhs);

    private:
        Material m_obj2attachment_mat = nullptr;
        Material m_quad_mat           = nullptr;
        Model    m_quad_model         = nullptr;

        std::shared_ptr<ImageData> m_color_attachment    = nullptr;
        std::shared_ptr<ImageData> m_normal_attachment   = nullptr;
        std::shared_ptr<ImageData> m_position_attachment = nullptr;

        LightDataBlock  m_LightDatas;
        LightSpawnBlock m_LightInfos;

        std::string       m_pass_names[2];
        BuiltinRenderStat m_render_stat[2];
    };
} // namespace Meow
