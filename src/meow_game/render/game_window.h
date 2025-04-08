#pragma once

#include "meow_runtime/function/object/game_object.h"
#include "meow_runtime/function/render/render_pass/shadow_map_pass.h"
#include "meow_runtime/function/window/graphics_window.h"
#include "render/render_pass/deferred_pass_game.h"
#include "render/render_pass/forward_pass_game.h"

namespace Meow
{
    class GameWindow : public GraphicsWindow
    {
    public:
        GameWindow(std::size_t id, GLFWwindow* glfw_window = nullptr);
        ~GameWindow() override;

        void Tick(float dt) override;

    private:
        void CreateRenderPass();
        void RecreateSwapChain();
        void RefreshFrameBuffers();

        ShadowMapPass    m_shadow_map_pass = nullptr;
        DeferredPassGame m_deferred_pass   = nullptr;
        ForwardPassGame  m_forward_pass    = nullptr;
        RenderPassBase*  m_render_pass_ptr = nullptr;

        // TODO: hard code render pass switching
        std::vector<std::weak_ptr<GameObject>> opaque_objects;
        std::vector<std::weak_ptr<GameObject>> translucent_objects;

        Signal<> m_wait_until_next_tick_signal;
    };
} // namespace Meow
