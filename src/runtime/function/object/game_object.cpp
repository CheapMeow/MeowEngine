#include "game_object.h"

#include "function/components/component.h"

namespace Meow
{
    void GameObject::Tick(float dt)
    {
        for (auto& refl_component : m_refl_components)
        {
            refl_component.shared_ptr->Tick(dt);
        }
    }

    bool GameObject::HasComponent(const std::string& compenent_type_name) const
    {
        for (const auto& refl_component : m_refl_components)
        {
            if (refl_component.type_name == compenent_type_name)
                return true;
        }

        return false;
    }
} // namespace Meow
