#pragma once

#include "meow_runtime/function/object/game_object.h"
#include "meow_runtime/function/render/render_pass/shadow_map_pass.h"
#include "meow_runtime/function/window/graphics_window.h"
#include "render/render_pass/deferred_pass_editor.h"
#include "render/render_pass/depth_to_color_pass.h"
#include "render/render_pass/forward_pass_editor.h"
#include "render/render_pass/imgui_pass.h"
#include "render/render_pass/shadow_coord_to_color_pass.h"

namespace Meow
{
    class EditorWindow : public GraphicsWindow
    {
    public:
        EditorWindow(std::size_t id, GLFWwindow* glfw_window = nullptr);
        ~EditorWindow() override;

        void Tick(float dt) override;

    private:
        void CreateRenderPass();
        void InitImGui();
        void RecreateSwapChain();
        void RefreshFrameBuffers();
        void BindImageToImguiPass();

        ShadowMapPass          m_shadow_map_pass            = nullptr;
        DepthToColorPass       m_depth_to_color_pass        = nullptr;
        ShadowCoordToColorPass m_shadow_coord_to_color_pass = nullptr;
        DeferredPassEditor     m_deferred_pass              = nullptr;
        ForwardPassEditor      m_forward_pass               = nullptr;
        ImGuiPass              m_imgui_pass                 = nullptr;
        RenderPassBase*        m_render_pass_ptr            = nullptr;

        // TODO: Dynamic descriptor pool?
        vk::raii::DescriptorPool m_imgui_descriptor_pool = nullptr;

        std::vector<std::shared_ptr<ImageData>> m_offscreen_render_targets;
        std::vector<vk::ImageView>              m_offscreen_render_target_image_views;
        bool                                    is_offscreen_valid = false;

        vk::Format                 m_depth_format               = vk::Format::eD16Unorm;
        std::shared_ptr<ImageData> m_depth_debugging_attachment = nullptr;

        // TODO: hard code render pass switching
        std::vector<std::weak_ptr<GameObject>> opaque_objects;
        std::vector<std::weak_ptr<GameObject>> translucent_objects;

        Signal<> m_wait_until_next_tick_signal;
    };
} // namespace Meow
