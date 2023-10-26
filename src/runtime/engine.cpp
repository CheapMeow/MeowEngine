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

        g_runtime_global_context.StartSystems();

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
        while (g_runtime_global_context.IsRunning())
        {
            float curr_time  = Time::GetTime();
            float frame_time = curr_time - m_last_time;

            g_runtime_global_context.m_window->Update(frame_time);
            g_runtime_global_context.m_render_system->Update(frame_time);
        }
    }

    void MeowEngine::ShutDown() { g_runtime_global_context.ShutDownSystems(); }
} // namespace Meow