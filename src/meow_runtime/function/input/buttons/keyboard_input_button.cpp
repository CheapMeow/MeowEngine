#include "keyboard_input_button.h"
#include "core/base/log.hpp"

namespace Meow
{
    KeyboardInputButton::KeyboardInputButton(std::shared_ptr<Window> window, KeyCode key)
        : m_key(key)
    {
        if (!window)
        {
            MEOW_ERROR("Try add KeyboardInputButton to window, but the window doesn't exist!");
            return;
        }

        m_window      = window;
        m_slot_id     = window->OnKey().connect([this](KeyCode key, InputAction action, uint8_t mods) {
            if (m_key == key)
            {
                m_on_button(action, mods);
            }
        });
        m_initialized = true;
    }

    KeyboardInputButton::~KeyboardInputButton()
    {
        if (!m_initialized)
            return;

        auto window = m_window.lock();
        if (!window)
        {
            MEOW_ERROR("Try get window in dector of KeyboardInputButton, but the window doesn't exist!");
            return;
        }

        if (!window->OnKey().disconnect(m_slot_id))
        {
            MEOW_ERROR("Try disconnect slot in dector of KeyboardInputButton, but the slot doesn't exist in window!");
        }
    }

    void swap(KeyboardInputButton& lhs, KeyboardInputButton& rhs)
    {
        using std::swap;

        swap(static_cast<InputButton&>(lhs), static_cast<InputButton&>(rhs));

        swap(lhs.m_key, rhs.m_key);
        swap(lhs.m_slot_id, rhs.m_slot_id);
        swap(lhs.m_initialized, rhs.m_initialized);
    }

    InputAction KeyboardInputButton::GetAction() const
    {
        if (!m_initialized)
        {
            MEOW_ERROR("KeyboardInputButton is not initialized! Return InputAction::Release.");
            return InputAction::Release;
        }

        auto window = m_window.lock();
        if (!window)
        {
            MEOW_ERROR(
                "Try get window in KeyboardInputButton, but the window doesn't exist! Return InputAction::Release.");
            return InputAction::Release;
        }

        return window->GetKeyAction(m_key);
    }
} // namespace Meow
