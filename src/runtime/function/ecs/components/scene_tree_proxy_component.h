#pragma once

#include "function/ecs/component.h"

#include <entt/entt.hpp>

#include <vector>

namespace Meow
{
    class SceneTreeProxyComponent : Component
    {
    public:
        std::vector<std::weak_ptr<entt::entity>> m_child_entity_ptrs;
    };

} // namespace Meow