#pragma once

#include "meow_runtime/function/object/game_object.h"
#include "meow_runtime/function/render/structs/per_frame_data.h"
#include "meow_runtime/function/render/structs/surface_data.h"
#include "meow_runtime/function/render/structs/swapchain_data.h"
#include "meow_runtime/function/window/window.h"
#include "render/render_pass/game_deferred_pass.h"
#include "render/render_pass/game_forward_pass.h"


namespace Meow
{
    class GameWindow : public Window
    {
    public:
        GameWindow(std::size_t id, GLFWwindow* glfw_window = nullptr);
        ~GameWindow() override;

        void Tick(float dt) override;

    private:
        void CreateSurface();
        void CreateSwapChian();
        void CreateDescriptorAllocator();
        void CreatePerFrameData();
        void CreateRenderPass();
        void RecreateSwapChain();
        void RefreshRenderPass();

        SwapChainData m_swapchain_data = nullptr;

        DescriptorAllocatorGrowable m_descriptor_allocator = nullptr;
        std::vector<PerFrameData>   m_per_frame_data;

        GameDeferredPass m_deferred_pass   = nullptr;
        GameForwardPass  m_forward_pass    = nullptr;
        RenderPass*      m_render_pass_ptr = nullptr;

        bool           m_framebuffer_resized  = false;
        bool           m_iconified            = false;
        const uint64_t k_fence_timeout        = 100000000;
        const uint32_t k_max_frames_in_flight = 2;
        uint32_t       m_current_frame_index  = 0;
        uint32_t       m_current_image_index  = 0;
    };
} // namespace Meow