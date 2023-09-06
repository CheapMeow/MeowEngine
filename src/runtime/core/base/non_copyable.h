#pragma once

#include "core/base/macro.h"

namespace Meow
{
    /**
     * @brief Class that removes the copy constructor and operator from derived classes, while leaving move.
     */
    class LIBRARY_API NonCopyable
    {
    protected:
        NonCopyable()          = default;
        virtual ~NonCopyable() = default;

    public:
        NonCopyable(const NonCopyable&)                = delete;
        NonCopyable(NonCopyable&&) noexcept            = default;
        NonCopyable& operator=(const NonCopyable&)     = delete;
        NonCopyable& operator=(NonCopyable&&) noexcept = default;
    };
} // namespace Meow
