#pragma once

#include "core/reflect/reflect_pointer.hpp"
#include "object_id_allocator.h"

namespace Meow
{
    class Component;

    class GameObject
    {
    public:
        GameObject(GameObjectID id)
            : m_id {id}
        {}
        virtual ~GameObject();

        virtual void tick(float delta_time);

        GameObjectID getID() const { return m_id; }

        void               setName(std::string name) { m_name = name; }
        const std::string& getName() const { return m_name; }

        bool hasComponent(const std::string& compenent_type_name) const;

        std::vector<reflect::refl_shared_ptr<Component>> getComponents() { return m_components; }

        template<typename TComponent>
        TComponent* tryGetComponent(const std::string& compenent_type_name)
        {
            for (auto& component : m_components)
            {
                if (component.type_name == compenent_type_name)
                {
                    return static_cast<TComponent*>(component.operator->());
                }
            }

            return nullptr;
        }

        template<typename TComponent>
        const TComponent* tryGetComponentConst(const std::string& compenent_type_name) const
        {
            for (const auto& component : m_components)
            {
                if (component.type_name == compenent_type_name)
                {
                    return static_cast<const TComponent*>(component.operator->());
                }
            }
            return nullptr;
        }

#define tryGetComponent(COMPONENT_TYPE)      tryGetComponent<COMPONENT_TYPE>(#COMPONENT_TYPE)
#define tryGetComponentConst(COMPONENT_TYPE) tryGetComponentConst<const COMPONENT_TYPE>(#COMPONENT_TYPE)

    protected:
        GameObjectID                                     m_id {k_invalid_gobject_id};
        std::string                                      m_name;
        std::vector<reflect::refl_shared_ptr<Component>> m_components;
    };
} // namespace Meow
