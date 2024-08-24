
#include "engine.h"

#include "pch.h"

#include "core/reflect/reflect.hpp"
#include "function/global/runtime_global_context.h"
#include "generated/register_all.h"

#include <iostream>

namespace Meow
{
    bool MeowEngine::Init()
    {
        RegisterAll();

        Log::Init();

        // TODO: Init Dependencies graph
        g_runtime_global_context.file_system     = std::make_shared<FileSystem>();
        g_runtime_global_context.resource_system = std::make_shared<ResourceSystem>();
        g_runtime_global_context.window_system   = std::make_shared<WindowSystem>();
        g_runtime_global_context.input_system    = std::make_shared<InputSystem>();
        g_runtime_global_context.render_system   = std::make_shared<RenderSystem>();
        g_runtime_global_context.level_system    = std::make_shared<LevelSystem>();

        return true;
    }

    bool MeowEngine::Start()
    {
        g_runtime_global_context.level_system->Start();
        g_runtime_global_context.file_system->Start();
        g_runtime_global_context.resource_system->Start();
        g_runtime_global_context.window_system->Start();
        g_runtime_global_context.input_system->Start();
        g_runtime_global_context.render_system->Start();

        return true;
    }

    void MeowEngine::Run()
    {
        while (g_runtime_global_context.running)
        {
            float curr_time = Time::GetTime();
            float dt        = curr_time - m_last_time;
            m_last_time     = curr_time;

            // TODO: Update Dependencies graph
            g_runtime_global_context.resource_system->Tick(dt);
            g_runtime_global_context.window_system->Tick(dt);
            g_runtime_global_context.input_system->Tick(dt);
            g_runtime_global_context.render_system->Tick(dt);
            g_runtime_global_context.level_system->Tick(dt);

            TimerSingleton::Get().Clear();
        }
    }

    void MeowEngine::ShutDown()
    {
        // TODO: ShutDown Dependencies graph
        g_runtime_global_context.level_system    = nullptr;
        g_runtime_global_context.resource_system = nullptr;
        g_runtime_global_context.render_system   = nullptr;
        g_runtime_global_context.input_system    = nullptr;
        g_runtime_global_context.window_system   = nullptr;
        g_runtime_global_context.file_system     = nullptr;
    }
} // namespace Meow