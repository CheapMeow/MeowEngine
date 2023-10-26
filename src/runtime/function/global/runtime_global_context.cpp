#include "runtime_global_context.h"

namespace Meow
{
    RuntimeGlobalContext g_runtime_global_context;

    /**
     * @brief Start all systems.
     *
     * Start sequence is important.
     */
    void RuntimeGlobalContext::StartSystems()
    {
        m_window = std::make_shared<Window>(0);
        m_window->OnClose().connect([&]() { m_running = false; });

        m_input_system  = std::make_shared<InputSystem>();
        m_file_system   = std::make_shared<FileSystem>();
        m_render_system = std::make_shared<RenderSystem>();
    }

    void RuntimeGlobalContext::ShutDownSystems()
    {
        m_input_system  = nullptr;
        m_render_system = nullptr;
        m_file_system   = nullptr;
    }
} // namespace Meow