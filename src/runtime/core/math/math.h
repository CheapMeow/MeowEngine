#include "core/base/macro.h"

namespace Meow
{
    class LIBRARY_API Math
    {
    public:
        /**
         * Used to floor the value if less than the min.
         * @tparam T The values type.
         * @param min The minimum value.
         * @param value The value.
         * @return Returns a value with deadband applied.
         */
        template<typename T = float>
        static T Deadband(const T& min, const T& value)
        {
            return std::fabs(value) >= std::fabs(min) ? value : 0.0f;
        }
    };
} // namespace Meow