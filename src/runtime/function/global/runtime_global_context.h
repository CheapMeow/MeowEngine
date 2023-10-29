#pragma once

#include "function/ecs/systems/file_system.h"
#include "function/ecs/systems/input/input_system.h"
#include "function/ecs/systems/render_system.h"
#include "function/ecs/systems/window_system.h"
#include "function/renderer/window.h"

#include <entt/entt.hpp>

#include <memory>

namespace Meow
{
    struct RuntimeGlobalContext
    {
        bool running = true;

        std::shared_ptr<Window>         window;
        std::shared_ptr<WindowSystem>   window_system;
        std::shared_ptr<InputSystem>    input_system;
        std::shared_ptr<FileSystem>     file_system;
        std::shared_ptr<VulkanRenderer> renderer;
        std::shared_ptr<RenderSystem>   render_system;

        entt::registry registry;
    };

    extern RuntimeGlobalContext g_runtime_global_context;
} // namespace Meow