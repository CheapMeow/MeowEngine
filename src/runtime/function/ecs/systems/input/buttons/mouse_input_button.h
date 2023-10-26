#pragma once

#include "function/ecs/systems/input/input_button.h"

namespace Meow
{
    class MouseInputButton : public InputButton
    {
    public:
        /**
         * @brief a new button mouse.
         * @param button The button on the mouse being checked.
         */
        explicit MouseInputButton(MouseButtonCode button = MouseButtonCode::Button0);

        InputAction GetAction() const override;

        MouseButtonCode GetButton() const { return m_button; }
        void            SetButton(MouseButtonCode button) { m_button = button; }

    private:
        MouseButtonCode m_button;
    };
} // namespace Meow
