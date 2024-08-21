
#include "engine.h"

#include "core/reflect/reflect.hpp"
#include "core/time/time.h"
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
        // while (m_Running)
        // {
        //     double currTime  = Time::GetTime();
        //     double frameTime = currTime - m_lastTime;
        //     if (frameTime > 0.25)
        //         frameTime = 0.25;
        //     m_lastTime = currTime;

        //     m_accumulator += frameTime;

        //     while (m_accumulator >= m_phyics_fixed_dt)
        //     {
        //         previousState = currentState;
        //         integrate(currentState, m_phyics_time, m_phyics_fixed_dt);
        //         m_phyics_time += m_phyics_fixed_dt;
        //         m_accumulator -= m_phyics_fixed_dt;
        //     }

        //     const double alpha = m_accumulator / m_phyics_fixed_dt;

        //     State state = currentState * alpha + previousState * (1.0 - alpha);

        //     render(state);
        // }
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