#pragma once

#include "function/input/input_enum.h"

#include "core/signal/signal.hpp"

namespace Meow
{
    class InputButton
    {
    public:
        virtual InputAction GetAction() const { return InputAction::Release; };

        /**
         * Called when the button changes state.
         * @return The delegate.
         */
        Signal<InputAction, uint8_t>& OnButton() { return m_on_button; }

    protected:
        Signal<InputAction, uint8_t> m_on_button;
    };
} // namespace Meow
