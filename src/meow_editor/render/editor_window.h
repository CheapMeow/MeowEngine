#pragma once

#include "core/signal/signal.hpp"
#include "meow_runtime/function/object/game_object.h"
#include "meow_runtime/function/render/buffer_data/per_frame_data.h"
#include "meow_runtime/function/render/buffer_data/surface_data.h"
#include "meow_runtime/function/render/buffer_data/swapchain_data.h"
#include "meow_runtime/function/render/render_pass/shadow_map_pass.h"
#include "meow_runtime/function/window/window.h"
#include "render/render_pass/deferred_pass_editor.h"
#include "render/render_pass/depth_to_color_pass.h"
#include "render/render_pass/forward_pass_editor.h"
#include "render/render_pass/imgui_pass.h"

namespace Meow
{
    class EditorWindow : public Window
    {
    public:
        EditorWindow(std::size_t id, GLFWwindow* glfw_window = nullptr);
        ~EditorWindow() override;

        void Tick(float dt) override;

    private:
        void CreateSwapChian();
        void CreatePerFrameData();
        void CreateRenderPass();
        void InitImGui();
        void RecreateSwapChain();
        void RefreshFrameBuffers();

        SwapChainData m_swapchain_data = nullptr;

        std::vector<PerFrameData> m_per_frame_data;

        ShadowMapPass      m_shadow_map_pass     = nullptr;
        DepthToColorPass   m_depth_to_color_pass = nullptr;
        DeferredPassEditor m_deferred_pass       = nullptr;
        ForwardPassEditor  m_forward_pass        = nullptr;
        ImGuiPass          m_imgui_pass          = nullptr;
        RenderPassBase*        m_render_pass_ptr     = nullptr;

        // TODO: Dynamic descriptor pool?
        vk::raii::DescriptorPool m_imgui_descriptor_pool = nullptr;

        bool           m_framebuffer_resized  = false;
        bool           m_iconified            = false;
        const uint64_t k_fence_timeout        = 100000000;
        const uint32_t k_max_frames_in_flight = 2;
        uint32_t       m_current_frame_index  = 0;
        uint32_t       m_current_image_index  = 0;

        std::shared_ptr<ImageData> m_offscreen_render_target;
        bool                       is_offscreen_valid = false;

        // TODO: hard code render pass switching
        std::vector<std::weak_ptr<GameObject>> opaque_objects;
        std::vector<std::weak_ptr<GameObject>> translucent_objects;

        Signal<> m_wait_until_next_tick_signal;
    };
} // namespace Meow
