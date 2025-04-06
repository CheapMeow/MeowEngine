#pragma once

#include "meow_runtime/core/signal/signal.hpp"
#include "meow_runtime/function/render/material/material.h"
#include "meow_runtime/function/render/material/shader.h"
#include "meow_runtime/function/render/render_pass/render_pass.h"
#include "render/imgui_widgets/builtin_statistics_widget.h"
#include "render/imgui_widgets/components_widget.h"
#include "render/imgui_widgets/flame_graph_widget.h"
#include "render/imgui_widgets/game_objects_widget.h"
#include "render/imgui_widgets/gizmo_widget.h"

namespace Meow
{
    class ImGuiPass : public RenderPass
    {
    public:
        ImGuiPass(std::nullptr_t);

        ImGuiPass(ImGuiPass&& rhs) noexcept
            : RenderPass(nullptr)
        {
            swap(*this, rhs);
        }

        ImGuiPass& operator=(ImGuiPass&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }
            return *this;
        }

        ImGuiPass(SurfaceData& surface_data);

        void RefreshFrameBuffers(const std::vector<vk::ImageView>& output_image_views,
                                 const vk::Extent2D&               extent) override;

        void Start(const vk::raii::CommandBuffer& command_buffer,
                   vk::Extent2D                   extent,
                   uint32_t                       current_image_index) override;

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        Signal<bool>& OnMSAAEnabledChanged() { return m_on_msaa_enabled_changed; }
        Signal<int>&  OnPassChanged() { return m_on_pass_changed; }

        friend void swap(ImGuiPass& lhs, ImGuiPass& rhs);

        void RefreshOffscreenRenderTarget(VkSampler image_sampler, VkImageView image_view, VkImageLayout image_layout);
        void RefreshShadowMap(VkSampler image_sampler, VkImageView image_view, VkImageLayout image_layout);

    private:
        bool         m_msaa_enabled = true;
        Signal<bool> m_on_msaa_enabled_changed;

        int                      m_cur_render_pass   = 1;
        std::vector<const char*> m_render_pass_names = {"Deferred", "Forward"};
        Signal<int>              m_on_pass_changed;

        bool            m_is_offscreen_image_valid = false;
        VkDescriptorSet m_offscreen_image_desc;

        VkDescriptorSet m_shadow_map_desc;

        GameObjectsWidget       m_gameobjects_widget;
        ComponentsWidget        m_components_widget;
        FlameGraphWidget        m_flame_graph_widget;
        BuiltinStatisticsWidget m_builtin_stat_widget;
        GizmoWidget             m_gizmo_widget;

        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;
    };
} // namespace Meow
