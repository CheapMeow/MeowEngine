#pragma once

#include "core/signal/signal.hpp"
#include "function/render/imgui_widgets/builtin_statistics_widget.h"
#include "function/render/imgui_widgets/components_widget.h"
#include "function/render/imgui_widgets/flame_graph_widget.h"
#include "function/render/imgui_widgets/game_objects_widget.h"
#include "function/render/render_pass/render_pass.h"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"

namespace Meow
{
    class ImGuiPass : public RenderPass
    {
    public:
        ImGuiPass(std::nullptr_t)
            : RenderPass(nullptr)
        {}

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

        ImGuiPass()
            : RenderPass()
        {
#ifdef MEOW_EDITOR
            m_pass_name = "ImGui Pass";
#endif
        }

        void DrawImGui();

        void Draw(const vk::raii::CommandBuffer& command_buffer) override;

        Signal<int>& OnPassChanged() { return m_on_pass_changed; }

        friend void swap(ImGuiPass& lhs, ImGuiPass& rhs);

        void RefreshOffscreenRenderTarget(VkSampler     offscreen_image_sampler,
                                          VkImageView   offscreen_image_view,
                                          VkImageLayout offscreen_image_layout);

    private:
        int                      m_cur_render_pass   = 0;
        std::vector<const char*> m_render_pass_names = {"Deferred", "Forward"};
        Signal<int>              m_on_pass_changed;

        VkDescriptorSet m_offscreen_image_desc;

        GameObjectsWidget       m_gameobjects_widget;
        ComponentsWidget        m_components_widget;
        FlameGraphWidget        m_flame_graph_widget;
        BuiltinStatisticsWidget m_builtin_stat_widget;
    };
} // namespace Meow