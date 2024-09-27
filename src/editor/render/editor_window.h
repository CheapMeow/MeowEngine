#pragma once

#include "runtime/function/object/game_object.h"
#include "runtime/function/render/render_pass/deferred_pass.h"
#include "runtime/function/render/render_pass/forward_pass.h"
#include "runtime/function/render/render_pass/imgui_pass.h"
#include "runtime/function/render/structs/per_frame_data.h"
#include "runtime/function/render/structs/surface_data.h"
#include "runtime/function/render/structs/swapchain_data.h"
#include "runtime/function/window/window.h"

namespace Meow
{
    class EditorWindow : public Window
    {
    public:
        EditorWindow(std::size_t id);
        ~EditorWindow() override;

        void Tick(float dt) override;

        const SurfaceData& GetSurfaceData() override { return m_surface_data; }

    private:
        void CreateSurface();
        void CreateSwapChian();
        void CreateDescriptorAllocator();
        void CreatePerFrameData();
        void CreateRenderPass();
        void InitImGui();
        void RecreateSwapChain();

        SwapChainData m_swapchain_data = nullptr;

        DescriptorAllocatorGrowable m_descriptor_allocator = nullptr;
        std::vector<PerFrameData>   m_per_frame_data;

        DeferredPass m_deferred_pass   = nullptr;
        ForwardPass  m_forward_pass    = nullptr;
        ImGuiPass    m_imgui_pass      = nullptr;
        RenderPass*  m_render_pass_ptr = nullptr;

        // TODO: Dynamic descriptor pool?
        vk::raii::DescriptorPool m_imgui_descriptor_pool = nullptr;

        bool           m_framebuffer_resized  = false;
        bool           m_iconified            = false;
        const uint64_t k_fence_timeout        = 100000000;
        const uint32_t k_max_frames_in_flight = 2;
        uint32_t       m_current_frame_index  = 0;
        uint32_t       m_current_image_index  = 0;
    };
} // namespace Meow