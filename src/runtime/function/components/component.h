#pragma once

namespace Meow
{
    struct Component
    {
        std::weak_ptr<GObject> m_parent_object
    };
} // namespace Meow