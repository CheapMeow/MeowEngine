#include "mouse_input_button.h"
#include "core/base/log.hpp"

namespace Meow
{
    MouseInputButton::MouseInputButton(std::shared_ptr<Window> window, MouseButtonCode button)
        : m_button(button)
    {
        if (!window)
        {
            MEOW_ERROR("Try add MouseInputButton to window, but the window doesn't exist!");
            return;
        }

        m_window  = window;
        m_slot_id = window->OnMouseButton().connect([this](MouseButtonCode button, InputAction action, uint8_t mods) {
            if (m_button == button)
            {
                m_on_button(action, mods);
            }
        });
        m_initialized = true;
    }

    MouseInputButton::~MouseInputButton()
    {
        if (!m_initialized)
            return;

        auto window = m_window.lock();
        if (!window)
        {
            MEOW_ERROR("Try get window in dector of MouseInputButton, but the window doesn't exist!");
            return;
        }

        if (!window->OnMouseButton().disconnect(m_slot_id))
        {
            MEOW_ERROR("Try disconnect slot in dector of MouseInputButton, but the slot doesn't exist in window!");
        }
    }

    void swap(MouseInputButton& lhs, MouseInputButton& rhs)
    {
        using std::swap;
        swap(lhs.m_button, rhs.m_button);
        swap(lhs.m_slot_id, rhs.m_slot_id);
        swap(lhs.m_initialized, rhs.m_initialized);
    }

    InputAction MouseInputButton::GetAction() const
    {
        if (!m_initialized)
        {
            MEOW_ERROR("MouseInputButton is not initialized! Return InputAction::Release.");
            return InputAction::Release;
        }

        auto window = m_window.lock();
        if (!window)
        {
            MEOW_ERROR(
                "Try get window in MouseInputButton, but the window doesn't exist! Return InputAction::Release.");
            return InputAction::Release;
        }

        return window->GetMouseButtonAction(m_button);
    }
} // namespace Meow
