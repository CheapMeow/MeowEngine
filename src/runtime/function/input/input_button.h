#pragma once

#include "function/input/input_enum.h"

#include <rocket.hpp>

namespace Meow
{
    class InputButton : public rocket::trackable
    {
    public:
        virtual InputAction GetAction() const { return InputAction::Release; };

        /**
         * Called when the button changes state.
         * @return The delegate.
         */
        rocket::signal<void(InputAction, uint8_t)>& OnButton() { return m_on_button; }

    protected:
        rocket::signal<void(InputAction, uint8_t)> m_on_button;
    };
} // namespace Meow
