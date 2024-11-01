#pragma once

#include "function/file/file_system.h"
#include "function/input/input_system.h"
#include "function/level/level_system.h"
#include "function/render/render_system.h"
#include "function/resource/resource_system.h"
#include "function/time/time_system.h"
#include "function/window/window_system.h"

#include <memory>

namespace Meow
{
    struct RuntimeGlobalContext
    {
        bool running = true;

        std::shared_ptr<TimeSystem>     time_system     = nullptr;
        std::shared_ptr<ResourceSystem> resource_system = nullptr;
        std::shared_ptr<WindowSystem>   window_system   = nullptr;
        std::shared_ptr<InputSystem>    input_system    = nullptr;
        std::shared_ptr<FileSystem>     file_system     = nullptr;
        std::shared_ptr<RenderSystem>   render_system   = nullptr;
        std::shared_ptr<LevelSystem>    level_system    = nullptr;
    };

    extern RuntimeGlobalContext g_runtime_global_context;
} // namespace Meow