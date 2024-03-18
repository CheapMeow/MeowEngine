#pragma once

#include <rocket.hpp>

namespace Meow
{
    class InputAxis : public rocket::trackable
    {
    public:
        /**
         * @brief the current value along the axis. -1 is smallest input, 1 is largest input.
         * @return The current value of the axis in the range (-1, 1).
         */
        virtual float GetAmount() const { return 0.0f; }

        /**
         * @brief Called when the axis changes value.
         * @return The delegate.
         */
        rocket::signal<void(float)>& OnAxis() { return m_on_axis_singal; }

        float GetScale() const { return m_scale; }
        void  SetScale(float scale) { m_scale = scale; }

        float GetOffset() const { return m_offset; }
        void  SetOffset(float offset) { m_offset = offset; }

    protected:
        rocket::signal<void(float)> m_on_axis_singal;
        float                       m_scale  = 1.0f;
        float                       m_offset = 0.0f;
    };
} // namespace Meow
