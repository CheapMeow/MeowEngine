#pragma once

#include "core/input/input_button.h"

namespace Meow
{
    class KeyboardInputButton : public InputButton
    {
    public:
        /**
         * @brief a new button keyboard.
         * @param m_key The m_key on the keyboard being checked.
         */
        explicit KeyboardInputButton(KeyCode key = KeyCode::Unknown);

        InputAction GetAction() const override;

        KeyCode GetKey() const { return m_key; }
        void    SetKey(KeyCode key) { m_key = key; }

    private:
        KeyCode m_key;
    };
} // namespace Meow
