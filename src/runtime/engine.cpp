#include "engine.h"
#include "core/log/log.h"
#include "core/time/time.h"
#include "function/global/runtime_global_context.h"

#include <iostream>

namespace Meow
{
    bool MeowEngine::Init()
    {
        Log::Init();

        g_runtime_global_context.resource_system = std::make_shared<ResourceSystem>();
        g_runtime_global_context.window_system   = std::make_shared<WindowSystem>();
        g_runtime_global_context.input_system    = std::make_shared<InputSystem>();
        g_runtime_global_context.file_system     = std::make_shared<FileSystem>();
        g_runtime_global_context.camera_system   = std::make_shared<CameraSystem>();
        g_runtime_global_context.render_system   = std::make_shared<RenderSystem>();

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

        //     while (m_accumulator >= m_phyics_fixed_delta_time)
        //     {
        //         previousState = currentState;
        //         integrate(currentState, m_phyics_time, m_phyics_fixed_delta_time);
        //         m_phyics_time += m_phyics_fixed_delta_time;
        //         m_accumulator -= m_phyics_fixed_delta_time;
        //     }

        //     const double alpha = m_accumulator / m_phyics_fixed_delta_time;

        //     State state = currentState * alpha + previousState * (1.0 - alpha);

        //     render(state);
        // }
        while (g_runtime_global_context.running)
        {
            float curr_time  = Time::GetTime();
            float frame_time = curr_time - m_last_time;
            m_last_time      = curr_time;

            g_runtime_global_context.resource_system->Update(frame_time);
            g_runtime_global_context.window_system->Update(frame_time);
            g_runtime_global_context.input_system->Update(frame_time);
            g_runtime_global_context.camera_system->Update(frame_time);
            g_runtime_global_context.render_system->Update(frame_time);
        }
    }

    void MeowEngine::ShutDown()
    {
        g_runtime_global_context.resource_system = nullptr;
        g_runtime_global_context.render_system   = nullptr;
        g_runtime_global_context.camera_system   = nullptr;
        g_runtime_global_context.file_system     = nullptr;
        g_runtime_global_context.input_system    = nullptr;
        g_runtime_global_context.window_system   = nullptr;
    }
} // namespace Meow