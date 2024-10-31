#pragma once

#include "function/render/structs/builtin_render_stat.h"
#include "function/render/structs/material.h"
#include "function/render/structs/model.h"
#include "function/render/structs/shader.h"
#include "meow_runtime/function/render/render_pass/render_pass.h"

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

    class GameDeferredPass : public RenderPass
    {
    public:
        GameDeferredPass(std::nullptr_t)
            : RenderPass(nullptr)
        {
        }

        GameDeferredPass(const vk::raii::PhysicalDevice& physical_device,
                         const vk::raii::Device&         device,
                         SurfaceData&                    surface_data,
                         const vk::raii::CommandPool&    command_pool,
                         const vk::raii::Queue&          queue,
                         DescriptorAllocatorGrowable&    m_descriptor_allocator);

        GameDeferredPass(GameDeferredPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        GameDeferredPass& operator=(GameDeferredPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ~GameDeferredPass() override
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

        friend void swap(GameDeferredPass& lhs, GameDeferredPass& rhs);

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