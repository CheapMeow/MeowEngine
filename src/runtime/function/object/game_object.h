#pragma once

#include "pch.h"

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

        virtual ~GameObject() { m_refl_components.clear(); }

        virtual void Tick(float dt);

        GameObjectID GetID() const { return m_id; }

        void               SetName(std::string name) { m_name = name; }
        const std::string& GetName() const { return m_name; }

        bool HasComponent(const std::string& compenent_type_name) const;

        std::vector<reflect::refl_shared_ptr<Component>> GetComponents() { return m_refl_components; }

        template<typename TComponent>
        std::weak_ptr<TComponent> TryGetComponent()
        {
            const std::string component_type_name = typeid(TComponent).name();

            for (auto& refl_component : m_refl_components)
            {
                if (refl_component.type_name == component_type_name)
                {
                    return std::dynamic_pointer_cast<TComponent>(refl_component.shared_ptr);
                }
            }

            return std::shared_ptr<TComponent>(nullptr);
        }

        template<typename TComponent>
        std::weak_ptr<TComponent> TryAddComponent(std::shared_ptr<TComponent> component_ptr)
        {
#ifdef MEOW_DEBUG
            if (!component_ptr)
            {
                RUNTIME_ERROR("shared ptr is invalid!");
                return std::shared_ptr<TComponent>(nullptr);
            }
#endif

            const std::string component_type_name = typeid(TComponent).name();

            // Check if a component of the same type already exists
            for (const auto& refl_component : m_refl_components)
            {
                if (refl_component.type_name == component_type_name)
                {
                    RUNTIME_ERROR("Component already exists: {}", component_type_name);
                    return std::shared_ptr<TComponent>(nullptr);
                }
            }

            // Add the component to the container
            m_refl_components.emplace_back(component_type_name, component_ptr);

#ifdef MEOW_DEBUG
            if (m_refl_components.size() < 1)
            {
                RUNTIME_ERROR("m_refl_components is empty!");
                return std::shared_ptr<TComponent>(nullptr);
            }
#endif

            return std::dynamic_pointer_cast<TComponent>(m_refl_components[m_refl_components.size() - 1].shared_ptr);
        }

    protected:
        GameObjectID                                     m_id {k_invalid_gobject_id};
        std::string                                      m_name;
        std::vector<reflect::refl_shared_ptr<Component>> m_refl_components;
    };
} // namespace Meow
