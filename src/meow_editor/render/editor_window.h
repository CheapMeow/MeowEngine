#pragma once

#include "meow_runtime/function/object/game_object.h"
#include "meow_runtime/function/render/structs/per_frame_data.h"
#include "meow_runtime/function/render/structs/surface_data.h"
#include "meow_runtime/function/render/structs/swapchain_data.h"
#include "meow_runtime/function/window/window.h"
#include "render/render_pass/editor_deferred_pass.h"
#include "render/render_pass/editor_forward_pass.h"
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
        void CreateSurface();
        void CreateSwapChian();
        void CreatePerFrameData();
        void CreateRenderPass();
#ifdef MEOW_EDITOR
        void InitImGui();
#endif
        void RecreateSwapChain();
        void RefreshRenderPass();

        SwapChainData m_swapchain_data = nullptr;

        std::vector<PerFrameData> m_per_frame_data;

        EditorDeferredPass m_deferred_pass = nullptr;
        EditorForwardPass  m_forward_pass  = nullptr;
#ifdef MEOW_EDITOR
        ImGuiPass m_imgui_pass = nullptr;
#endif
        RenderPass* m_render_pass_ptr = nullptr;

#ifdef MEOW_EDITOR
        // TODO: Dynamic descriptor pool?
        vk::raii::DescriptorPool m_imgui_descriptor_pool = nullptr;
#endif

        bool           m_framebuffer_resized  = false;
        bool           m_iconified            = false;
        const uint64_t k_fence_timeout        = 100000000;
        const uint32_t k_max_frames_in_flight = 2;
        uint32_t       m_current_frame_index  = 0;
        uint32_t       m_current_image_index  = 0;

#ifdef MEOW_EDITOR
        std::shared_ptr<ImageData> m_offscreen_render_target;
        bool                       is_offscreen_valid = false;
#endif

        // TODO: hard code render pass switching
        std::vector<std::weak_ptr<GameObject>> opaque_objects;
        std::vector<std::weak_ptr<GameObject>> translucent_objects;
    };
} // namespace Meow
