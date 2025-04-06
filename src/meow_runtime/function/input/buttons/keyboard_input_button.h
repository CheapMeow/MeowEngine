#pragma once

#include "function/input/input_button.h"

#include <iostream>

namespace Meow
{
    class KeyboardInputButton : public InputButton
    {
    public:
        /**
         * @brief a new button keyboard.
         * @param m_key The m_key on the keyboard being checked.
         */
        explicit KeyboardInputButton(std::shared_ptr<Window> window, KeyCode key = KeyCode::Unknown);
        ~KeyboardInputButton();

        KeyboardInputButton(KeyboardInputButton&& rhs) noexcept
            : InputButton(nullptr)
        {
            swap(*this, rhs);
        }

        KeyboardInputButton& operator=(KeyboardInputButton&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);

                std::cout << "Hello!" << std::endl;
            }
            return *this;
        }

        friend void swap(KeyboardInputButton& lhs, KeyboardInputButton& rhs);

        InputAction GetAction() const override;

        KeyCode GetKey() const { return m_key; }
        void    SetKey(KeyCode key) { m_key = key; }

    private:
        KeyCode     m_key;
        std::size_t m_slot_id     = 0;
        bool        m_initialized = false;
    };
} // namespace Meow
