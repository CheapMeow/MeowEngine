
#include "runtime.h"

#include "pch.h"

#include "core/reflect/reflect.hpp"
#include "function/global/runtime_context.h"
#include "generated/register_all.h"

#include <iostream>

namespace Meow
{
    bool MeowRuntime::Init()
    {
        RegisterAll();

        // TODO: Init Dependencies graph
        g_runtime_context.time_system     = std::make_shared<TimeSystem>();
        g_runtime_context.file_system     = std::make_shared<FileSystem>();
        g_runtime_context.resource_system = std::make_shared<ResourceSystem>();
        g_runtime_context.window_system   = std::make_shared<WindowSystem>();
        g_runtime_context.render_system   = std::make_shared<RenderSystem>();
        g_runtime_context.input_system    = std::make_shared<InputSystem>();
        g_runtime_context.level_system    = std::make_shared<LevelSystem>();

        return true;
    }

    bool MeowRuntime::Start()
    {
        g_runtime_context.time_system->Start();
        g_runtime_context.level_system->Start();
        g_runtime_context.file_system->Start();
        g_runtime_context.resource_system->Start();
        g_runtime_context.render_system->Start();
        g_runtime_context.window_system->Start();
        g_runtime_context.input_system->Start();

        return true;
    }

    void MeowRuntime::Tick(float dt)
    {
        // TODO: Update Dependencies graph
        g_runtime_context.time_system->Tick(dt);
        g_runtime_context.resource_system->Tick(dt);
        g_runtime_context.window_system->Tick(dt);
        g_runtime_context.input_system->Tick(dt);
        g_runtime_context.render_system->Tick(dt);
        g_runtime_context.level_system->Tick(dt);

        TimerSingleton::Get().Clear();
    }

    void MeowRuntime::ShutDown()
    {
        // TODO: ShutDown Dependencies graph
        g_runtime_context.level_system    = nullptr;
        g_runtime_context.resource_system = nullptr;
        g_runtime_context.input_system    = nullptr;
        g_runtime_context.window_system   = nullptr;
        g_runtime_context.render_system   = nullptr;
        g_runtime_context.file_system     = nullptr;
        g_runtime_context.time_system     = nullptr;
    }
} // namespace Meow