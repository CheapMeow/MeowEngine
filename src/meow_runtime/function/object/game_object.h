#pragma once

#include "pch.h"

#include "core/reflect/reflect_pointer.hpp"
#include "core/uuid/uuid.h"

namespace Meow
{
    class GameObject;

    class Component
    {
    public:
        std::weak_ptr<GameObject> m_parent_object;

        virtual void Start() {};
        virtual void Tick(float dt) {};
    };

    class GameObject
    {
    public:
        std::weak_ptr<GameObject> self_weak_ptr;

        GameObject(UUID id)
            : m_id {id}
        {}

        virtual ~GameObject() { m_refl_components.clear(); }

        virtual void Tick(float dt);

        UUID GetID() const { return m_id; }

        void               SetName(std::string name) { m_name = name; }
        const std::string& GetName() const { return m_name; }

        bool HasComponent(const std::string& compenent_type_name) const;

        std::vector<reflect::refl_shared_ptr<Component>> GetComponents() { return m_refl_components; }

        template<typename TComponent>
        std::shared_ptr<TComponent> TryGetComponent(const std::string& component_type_name)
        {
            FUNCTION_TIMER();

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
        std::shared_ptr<TComponent> TryAddComponent(const std::string&          component_type_name,
                                                    std::shared_ptr<TComponent> component_ptr)
        {
            FUNCTION_TIMER();

#ifdef MEOW_DEBUG
            if (!component_ptr)
            {
                MEOW_ERROR("shared ptr is invalid!");
                return std::shared_ptr<TComponent>(nullptr);
            }
#endif

            // Check if a component of the same type already exists
            for (const auto& refl_component : m_refl_components)
            {
                if (refl_component.type_name == component_type_name)
                {
                    MEOW_ERROR("Component already exists: {}", component_type_name);
                    return std::shared_ptr<TComponent>(nullptr);
                }
            }

            // Add the component to the container
            m_refl_components.emplace_back(component_type_name, component_ptr);

#ifdef MEOW_DEBUG
            if (m_refl_components.size() < 1)
            {
                MEOW_ERROR("m_refl_components is empty!");
                return std::shared_ptr<TComponent>(nullptr);
            }
#endif

            // set parent gameobject

            component_ptr->m_parent_object = self_weak_ptr;
            component_ptr->Start();

            return component_ptr;
        }

    protected:
        UUID                                             m_id;
        std::string                                      m_name = "Default Object";
        std::vector<reflect::refl_shared_ptr<Component>> m_refl_components;
    };
} // namespace Meow
