#include "mouse_input_axis.h"
#include "core/base/log.hpp"

#include <glm/glm.hpp>

namespace Meow
{
    MouseInputAxis::MouseInputAxis(std::shared_ptr<Window> window, uint8_t axis)
        : m_axis(axis)
    {
        if (!window)
        {
            MEOW_ERROR("Try add MouseInputAxis to window, but the window doesn't exist!");
            return;
        }

        m_window      = window;
        m_slot_id     = window->OnMousePosition().connect([this](glm::vec2 value) { m_on_axis_signal(GetAmount()); });
        m_initialized = true;
    }

    MouseInputAxis::~MouseInputAxis()
    {
        if (!m_initialized)
            return;

        auto window = m_window.lock();
        if (!window)
        {
            MEOW_ERROR("Try get window in dector of MouseInputAxis, but the window doesn't exist!");
            return;
        }

        if (!window->OnMousePosition().disconnect(m_slot_id))
        {
            MEOW_ERROR("Try disconnect slot in dector of MouseInputAxis, but the slot doesn't exist in window!");
        }
    }

    void swap(MouseInputAxis& lhs, MouseInputAxis& rhs)
    {
        using std::swap;

        swap(static_cast<InputAxis&>(lhs), static_cast<InputAxis&>(rhs));

        swap(lhs.m_axis, rhs.m_axis);
        swap(lhs.m_slot_id, rhs.m_slot_id);
        swap(lhs.m_initialized, rhs.m_initialized);
    }

    float MouseInputAxis::GetAmount() const
    {
        if (!m_initialized)
        {
            MEOW_ERROR("MouseInputAxis is not initialized! Return 0.0f.");
            return 0.0f;
        }

        auto window = m_window.lock();
        if (!window)
        {
            MEOW_ERROR("Try get window in MouseInputAxis, but the window doesn't exist! Return 0.0f.");
            return 0.0f;
        }

        return m_scale * static_cast<float>(window->GetMousePositionDelta()[m_axis]) + m_offset;
    }
} // namespace Meow
