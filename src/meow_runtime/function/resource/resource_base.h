#pragma once

#include "core/base/non_copyable.h"
#include "core/uuid/uuid.h"

namespace Meow
{
    class ResourceBase : public NonCopyable
    {
    public:
        UUID uuid() { return m_uuid; }

    private:
        UUID m_uuid;
    };

} // namespace Meow
