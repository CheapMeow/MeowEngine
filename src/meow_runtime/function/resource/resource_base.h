#pragma once

#include "core/base/non_copyable.h"
#include "core/uuid/uuid.h"

namespace Meow
{
    struct ResourceBase : public NonCopyable
    {
        UUID uuid;
    };

} // namespace Meow
