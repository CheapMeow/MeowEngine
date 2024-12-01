#pragma once

#include "function/object/game_object.h"
#include "function/render/render_path/deferred_path.h"
#include "function/render/render_path/forward_path.h"
#include "function/render/structs/per_frame_data.h"
#include "function/render/structs/surface_data.h"
#include "function/render/structs/swapchain_data.h"
#include "function/window/window.h"

namespace Meow
{
    class RuntimeWindow : public Window
    {
    public:
        RuntimeWindow(std::size_t id, GLFWwindow* glfw_window = nullptr);
        ~RuntimeWindow() override;

        void Tick(float dt) override;

        void RefreshAttachments();

    private:
        void CreateSurface();
        void CreateSwapChian();
        void CreateDescriptorAllocator();
        void CreatePerFrameData();
        void CreateRenderPass();
        void RecreateSwapChain();

        SwapChainData             m_swapchain_data = nullptr;
        std::vector<PerFrameData> m_per_frame_data;
        bool                      m_framebuffer_resized  = false;
        bool                      m_iconified            = false;
        const uint64_t            k_fence_timeout        = 100000000;
        const uint32_t            k_max_frames_in_flight = 2;
        uint32_t                  m_current_frame_index  = 0;
        uint32_t                  m_current_image_index  = 0;

        ForwardPath  m_forward_path;
        DeferredPath m_deferred_path;
        int          m_cur_render_path = 0;
    };
} // namespace Meow
