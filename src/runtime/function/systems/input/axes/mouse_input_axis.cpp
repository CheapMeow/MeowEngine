#include "mouse_input_axis.h"

#include "function/global/runtime_global_context.h"

#include <glm/glm.hpp>

namespace Meow
{
    MouseInputAxis::MouseInputAxis(uint8_t axis)
        : axis(axis)
    {
        g_runtime_global_context.window_system->m_window->OnMousePosition().connect(
            this, [this](glm::vec2 value) { m_on_axis_singal(GetAmount()); });
    }

    float MouseInputAxis::GetAmount() const
    {
        return m_scale *
                   static_cast<float>(g_runtime_global_context.window_system->m_window->GetMousePositionDelta()[axis]) +
               m_offset;
    }
} // namespace Meow
