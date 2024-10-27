#pragma once

#include "function/input/input_button.h"

namespace Meow
{
    class MouseInputButton : public InputButton
    {
    public:
        /**
         * @brief a new button mouse.
         * @param button The button on the mouse being checked.
         */
        explicit MouseInputButton(std::shared_ptr<Window> window, MouseButtonCode button = MouseButtonCode::Button0);
        ~MouseInputButton();

        MouseInputButton(MouseInputButton&& rhs) noexcept
            : InputButton(std::move(rhs))
        {
            swap(*this, rhs);
        }

        MouseInputButton& operator=(MouseInputButton&& rhs) noexcept
        {
            if (this != &rhs)
            {
                InputButton::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        friend void LIBRARY_API swap(MouseInputButton& lhs, MouseInputButton& rhs);

        InputAction GetAction() const override;

        MouseButtonCode GetButton() const { return m_button; }
        void            SetButton(MouseButtonCode button) { m_button = button; }

    private:
        MouseButtonCode m_button;
        std::size_t     m_slot_id     = 0;
        bool            m_initialized = false;
    };
} // namespace Meow
