#pragma once

#include "window.h"

#include "function/render/buffer_data/per_frame_data.h"
#include "function/render/buffer_data/surface_data.h"
#include "function/render/buffer_data/swapchain_data.h"

namespace Meow
{
    class GraphicsWindow : public Window
    {
    public:
        GraphicsWindow(std::nullptr_t)
            : Window(nullptr)
        {}

        GraphicsWindow(std::size_t id, GLFWwindow* glfw_window = nullptr)
            : Window(id, glfw_window)
        {}

        virtual ~GraphicsWindow() override {}

        void               CreateSurface();
        const SurfaceData& GetSurfaceData() { return m_surface_data; }
        const vk::Format   GetColorFormat() { return m_color_format; }
        void               CreateSwapChian();
        void               CreatePerFrameData();

    protected:
        SurfaceData               m_surface_data = nullptr;
        vk::Format                m_color_format;
        SwapChainData             m_swapchain_data = nullptr;
        std::vector<PerFrameData> m_per_frame_data;

        bool           m_framebuffer_resized  = false;
        bool           m_iconified            = false;
        const uint64_t k_fence_timeout        = 100000000;
        const uint32_t k_max_frames_in_flight = 2;
        uint32_t       m_current_frame_index  = 0;
        uint32_t       m_current_image_index  = 0;
    };
} // namespace Meow