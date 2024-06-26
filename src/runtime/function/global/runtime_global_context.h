#pragma once

#include "function/camera/camera_system.h"
#include "function/file/file_system.h"
#include "function/input/input_system.h"
#include "function/render/render_system.h"
#include "function/resource/resource_system.h"
#include "function/window/window_system.h"

#include <entt/entt.hpp>

#include <memory>

namespace Meow
{
    struct RuntimeGlobalContext
    {
        bool running = true;

        entt::registry registry;

        std::shared_ptr<ResourceSystem> resource_system = nullptr;
        std::shared_ptr<WindowSystem>   window_system   = nullptr;
        std::shared_ptr<InputSystem>    input_system    = nullptr;
        std::shared_ptr<FileSystem>     file_system     = nullptr;
        std::shared_ptr<CameraSystem>   camera_system   = nullptr;
        std::shared_ptr<RenderSystem>   render_system   = nullptr;
    };

    extern RuntimeGlobalContext g_runtime_global_context;
} // namespace Meow