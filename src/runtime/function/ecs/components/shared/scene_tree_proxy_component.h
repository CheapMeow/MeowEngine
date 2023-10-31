#pragma once

#include "function/ecs/component.h"

#include <entt/entt.hpp>

#include <vector>

namespace Meow
{
    struct SceneTreeProxyComponent : Component
    {
        std::vector<std::weak_ptr<entt::entity>> m_child_entity_ptrs;
    };

} // namespace Meow