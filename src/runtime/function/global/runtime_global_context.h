#pragma once

#include "function/systems/camera/camera_system.h"
#include "function/systems/file/file_system.h"
#include "function/systems/input/input_system.h"
#include "function/systems/render/render_system.h"
#include "function/systems/window/window_system.h"
#include "render_context.h"

#include <entt/entt.hpp>

#include <memory>

namespace Meow
{
    struct RuntimeGlobalContext
    {
        bool running = true;

        RenderContext  render_context;
        entt::registry registry;

        std::shared_ptr<WindowSystem> window_system;
        std::shared_ptr<InputSystem>  input_system;
        std::shared_ptr<FileSystem>   file_system;
        std::shared_ptr<CameraSystem> camera_system;
        std::shared_ptr<RenderSystem> render_system;
    };

    extern RuntimeGlobalContext g_runtime_global_context;
} // namespace Meow