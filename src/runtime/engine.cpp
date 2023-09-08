#include "engine.h"
#include "core/log/log.h"
#include "core/time/time.h"
#include "function/renderer/window.h"

#include <iostream>

namespace Meow
{
    bool MeowEngine::Init()
    {
        Log::Init();

        m_window = std::make_shared<Window>(0);

        m_window->OnClose().connect([&]() { m_running = false; });

        m_renderer = std::make_unique<VulkanRenderer>(m_window);

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
        while (m_running)
        {
            double currTime  = Time::GetTime();
            double frameTime = currTime - m_lastTime;

            m_window->Update(frameTime);
        }
    }

    void MeowEngine::ShutDown() {}
} // namespace Meow