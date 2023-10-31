#pragma once

#include "core/input/input_axis.h"

namespace Meow
{
    class MouseInputAxis : public InputAxis
    {
    public:
        /**
         * @brief a new axis mouse.
         * @param axis The axis on the mouse delta is being checked.
         */
        explicit MouseInputAxis(uint8_t axis = 0);

        float GetAmount() const override;

    private:
        uint8_t axis;
    };
} // namespace Meow
