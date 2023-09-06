#include "engine.h"
#include "core/log/log.h"
#include "core/time/time.h"
#include "core/window/window.h"

#include <iostream>

namespace Meow
{
    bool MeowEngine::Init()
    {
        Log::Init();

        m_Window.reset(new Window(0));

        return true;
    }

    void MeowEngine::Run()
    {
        // while (m_Running)
        // {
        //     double currTime  = Time::GetTime();
        //     double frameTime = currTime - m_LastTime;
        //     if (frameTime > 0.25)
        //         frameTime = 0.25;
        //     m_LastTime = currTime;

        //     m_Accumulator += frameTime;

        //     while (m_Accumulator >= m_PhyicsFixedDeltaTime)
        //     {
        //         previousState = currentState;
        //         integrate(currentState, m_PhyicsTime, m_PhyicsFixedDeltaTime);
        //         m_PhyicsTime += m_PhyicsFixedDeltaTime;
        //         m_Accumulator -= m_PhyicsFixedDeltaTime;
        //     }

        //     const double alpha = m_Accumulator / m_PhyicsFixedDeltaTime;

        //     State state = currentState * alpha + previousState * (1.0 - alpha);

        //     render(state);
        // }
        while (m_Running)
        {
            double currTime  = Time::GetTime();
            double frameTime = currTime - m_LastTime;

            m_Window->Update(frameTime);
        }
    }

    void MeowEngine::ShutDown() {}
} // namespace Meow