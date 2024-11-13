#pragma once

#include "meow_runtime/core/signal/signal.hpp"
#include "meow_runtime/function/render/render_pass/render_pass.h"
#include "meow_runtime/function/render/structs/material.h"
#include "meow_runtime/function/render/structs/shader.h"
#include "render/imgui_widgets/builtin_statistics_widget.h"
#include "render/imgui_widgets/components_widget.h"
#include "render/imgui_widgets/flame_graph_widget.h"
#include "render/imgui_widgets/game_objects_widget.h"

namespace Meow
{
    class ImGuiPass : public RenderPass
    {
    public:
        ImGuiPass(std::nullptr_t);

        ImGuiPass(ImGuiPass&& rhs) noexcept
            : RenderPass(std::move(rhs))
        {
            swap(*this, rhs);
        }

        ImGuiPass& operator=(ImGuiPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                RenderPass::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        ImGuiPass(vk::raii::PhysicalDevice const& physical_device,
                  vk::raii::Device const&         device,
                  SurfaceData&                    surface_data,
                  vk::raii::CommandPool const&    command_pool,
                  vk::raii::Queue const&          queue,
                  DescriptorAllocatorGrowable&    m_descriptor_allocator);

        void RefreshFrameBuffers(vk::raii::PhysicalDevice const&   physical_device,
                                 vk::raii::Device const&           device,
                                 vk::raii::CommandPool const&      command_pool,
                                 vk::raii::Queue const&            queue,
                                 const std::vector<vk::ImageView>& output_image_views,
                                 vk::Extent2D const&               extent) override;

        void Start(vk::raii::CommandBuffer const& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void Draw(vk::raii::CommandBuffer const& command_buffer) override;

        Signal<int>& OnPassChanged() { return m_on_pass_changed; }

        friend void swap(ImGuiPass& lhs, ImGuiPass& rhs);

        void RefreshOffscreenRenderTarget(VkSampler     offscreen_image_sampler,
                                          VkImageView   offscreen_image_view,
                                          VkImageLayout offscreen_image_layout);

    private:
        int                      m_cur_render_pass   = 0;
        std::vector<const char*> m_render_pass_names = {"Deferred", "Forward"};
        Signal<int>              m_on_pass_changed;

        bool            m_is_offscreen_image_valid = false;
        VkDescriptorSet m_offscreen_image_desc;

        GameObjectsWidget       m_gameobjects_widget;
        ComponentsWidget        m_components_widget;
        FlameGraphWidget        m_flame_graph_widget;
        BuiltinStatisticsWidget m_builtin_stat_widget;

        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;
    };
} // namespace Meow
