#pragma once

#include "function/ecs/systems/file_system.h"
#include "function/ecs/systems/input/input_system.h"
#include "function/ecs/systems/render_system.h"
#include "function/renderer/window.h"

#include <memory>

namespace Meow
{
    class RuntimeGlobalContext
    {
    public:
        void StartSystems();
        void ShutDownSystems();

        bool IsRunning() { return m_running; }

        bool m_running = true;

        std::shared_ptr<Window> m_window;

        std::shared_ptr<InputSystem>  m_input_system;
        std::shared_ptr<FileSystem>   m_file_system;
        std::shared_ptr<RenderSystem> m_render_system;
    };

    extern RuntimeGlobalContext g_runtime_global_context;
} // namespace Meow