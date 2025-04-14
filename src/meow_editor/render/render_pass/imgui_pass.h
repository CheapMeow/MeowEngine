#pragma once

#include "meow_runtime/core/signal/signal.hpp"
#include "meow_runtime/function/render/material/material.h"
#include "meow_runtime/function/render/material/shader.h"
#include "meow_runtime/function/render/render_pass/render_pass_base.h"
#include "render/imgui_widgets/builtin_statistics_widget.h"
#include "render/imgui_widgets/components_widget.h"
#include "render/imgui_widgets/flame_graph_widget.h"
#include "render/imgui_widgets/game_objects_widget.h"
#include "render/imgui_widgets/gizmo_widget.h"

namespace Meow
{
    class ImGuiPass : public RenderPassBase
    {
    public:
        ImGuiPass(std::nullptr_t);

        ImGuiPass(ImGuiPass&& rhs) noexcept
            : RenderPassBase(nullptr)
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

        void Start(const vk::raii::CommandBuffer& command_buffer, vk::Extent2D extent, uint32_t image_index) override;

        void RecordGraphicsCommand(const vk::raii::CommandBuffer& command_buffer, uint32_t frame_index) override;

        Signal<bool>& OnMSAAEnabledChanged() { return m_on_msaa_enabled_changed; }
        Signal<int>&  OnPassChanged() { return m_on_pass_changed; }

        friend void swap(ImGuiPass& lhs, ImGuiPass& rhs);

        void RefreshOffscreenRenderTarget(std::vector<std::shared_ptr<ImageData>>& offscreen_render_targets,
                                          VkImageLayout                            image_layout);
        void RefreshShadowMap(VkSampler image_sampler, VkImageView image_view, VkImageLayout image_layout);
        void RefreshShadowCoord(VkSampler image_sampler, VkImageView image_view, VkImageLayout image_layout);
        void RefreshShadowDepth(VkSampler image_sampler, VkImageView image_view, VkImageLayout image_layout);

    private:
        bool         m_msaa_enabled = true;
        Signal<bool> m_on_msaa_enabled_changed;

        int                      m_cur_render_pass   = 1;
        std::vector<const char*> m_render_pass_names = {"Deferred", "Forward"};
        Signal<int>              m_on_pass_changed;

        bool                         m_is_offscreen_image_valid = false;
        std::vector<VkDescriptorSet> m_offscreen_image_descs;

        VkDescriptorSet m_shadow_map_desc;
        VkDescriptorSet m_shadow_coord_desc;
        VkDescriptorSet m_shadow_depth_desc;

        GameObjectsWidget       m_gameobjects_widget;
        ComponentsWidget        m_components_widget;
        FlameGraphWidget        m_flame_graph_widget;
        BuiltinStatisticsWidget m_builtin_stat_widget;
        GizmoWidget             m_gizmo_widget;

        bool                m_query_enabled = true;
        vk::raii::QueryPool query_pool      = nullptr;
    };
} // namespace Meow
