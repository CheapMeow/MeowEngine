#pragma once

#include "component.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace Meow
{
    class Entity
    {
    public:
        Entity() : m_name("None"), m_parent(nullptr), m_uuid(-1) {}

        ~Entity()
        {
            for (int32_t i = 0; i < m_children.size(); ++i)
            {
                delete m_children[i];
            }
            m_children.clear();
        }

    protected:
        std::string m_name;

        Entity*              m_parent = nullptr;
        std::vector<Entity*> m_children;

        std::vector<Component> m_components;

        int32_t m_uuid;
    };
} // namespace Meow
