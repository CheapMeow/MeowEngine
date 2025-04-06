#pragma once

#include "core/base/non_copyable.h"
#include "core/signal/signal.hpp"
#include "function/window/window.h"

#include <memory>

namespace Meow
{
    class InputAxis : public NonCopyable
    {
    public:
        InputAxis(std::nullptr_t) {}
        InputAxis() = default;

        /**
         * @brief the current value along the axis. -1 is smallest input, 1 is largest input.
         * @return The current value of the axis in the range (-1, 1).
         */
        virtual float GetAmount() const { return 0.0f; }

        /**
         * @brief Called when the axis changes value.
         * @return The delegate.
         */
        Signal<float>& OnAxis() { return m_on_axis_signal; }

        float GetScale() const { return m_scale; }
        void  SetScale(float scale) { m_scale = scale; }

        float GetOffset() const { return m_offset; }
        void  SetOffset(float offset) { m_offset = offset; }

    protected:
        std::weak_ptr<Window> m_window;
        Signal<float>         m_on_axis_signal;
        float                 m_scale  = 1.0f;
        float                 m_offset = 0.0f;
    };
} // namespace Meow
