#include "keyboard_input_button.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    KeyboardInputButton::KeyboardInputButton(KeyCode key)
        : m_key(key)
    {
        g_runtime_global_context.window->OnKey().connect(this, [this](KeyCode key, InputAction action, uint8_t mods) {
            if (m_key == key)
            {
                m_on_button(action, mods);
            }
        });
    }

    InputAction KeyboardInputButton::GetAction() const { return g_runtime_global_context.window->GetKeyAction(m_key); }
} // namespace Meow
