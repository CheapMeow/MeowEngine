#pragma once

#include "function/input/input_axis.h"

#include <cstdint>

namespace Meow
{
    class MouseInputAxis : public InputAxis
    {
    public:
        /**
         * @brief a new axis mouse.
         * @param axis The axis on the mouse delta is being checked.
         */
        explicit MouseInputAxis(std::shared_ptr<Window> window, uint8_t axis = 0);
        ~MouseInputAxis();

        MouseInputAxis(MouseInputAxis&& rhs) noexcept
            : InputAxis(std::move(rhs))
        {
            swap(*this, rhs);
        }

        MouseInputAxis& operator=(MouseInputAxis&& rhs) noexcept
        {
            if (this != &rhs)
            {
                InputAxis::operator=(std::move(rhs));

                swap(*this, rhs);
            }
            return *this;
        }

        friend void swap(MouseInputAxis& lhs, MouseInputAxis& rhs);

        float GetAmount() const override;

    private:
        uint8_t     m_axis;
        std::size_t m_slot_id     = 0;
        bool        m_initialized = false;
    };
} // namespace Meow
