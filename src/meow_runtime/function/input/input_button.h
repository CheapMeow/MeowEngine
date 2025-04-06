#pragma once

#include "core/base/non_copyable.h"
#include "core/signal/signal.hpp"
#include "function/input/input_enum.h"
#include "function/window/window.h"

#include <memory>

namespace Meow
{
    class InputButton : public NonCopyable
    {
    public:
        InputButton(std::nullptr_t) {}
        InputButton() = default;

        virtual InputAction GetAction() const { return InputAction::Release; };

        /**
         * Called when the button changes state.
         * @return The delegate.
         */
        Signal<InputAction, uint8_t>& OnButton() { return m_on_button; }

    protected:
        std::weak_ptr<Window>        m_window;
        Signal<InputAction, uint8_t> m_on_button;
    };
} // namespace Meow
