#pragma once

#include "core/signal/signal.hpp"
#include "function/render/imgui_widgets/components_widget.h"
#include "function/render/imgui_widgets/flame_graph_widget.h"
#include "function/render/structs/material.h"
#include "function/render/structs/shader.h"
#include "render_pass.h"

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

        void RefreshFrameBuffers(vk::raii::PhysicalDevice const&         physical_device,
                                 vk::raii::Device const&                 device,
                                 vk::raii::CommandBuffer const&          command_buffer,
                                 SurfaceData&                            surface_data,
                                 std::vector<vk::raii::ImageView> const& swapchain_image_views,
                                 vk::Extent2D const&                     extent) override;

        void Start(vk::raii::CommandBuffer const& command_buffer,
                   Meow::SurfaceData const&       surface_data,
                   uint32_t                       current_image_index) override;

        void Draw(vk::raii::CommandBuffer const& command_buffer) override;

        Signal<int>& OnPassChanged() { return m_on_pass_changed; }

        friend void swap(ImGuiPass& lhs, ImGuiPass& rhs);

    private:
        int                      m_cur_render_pass   = 0;
        std::vector<const char*> m_render_pass_names = {"Deferred", "Forward"};
        Signal<int>              m_on_pass_changed;

        ComponentsWidget m_components_widget;
        FlameGraphWidget m_flame_graph_widget;
    };
} // namespace Meow
