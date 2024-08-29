#include "input_scheme.h"

namespace Meow
{
    InputAxis* InputScheme::GetAxis(const std::string& name)
    {
        auto it = m_axes.find(name);
        if (it == m_axes.end())
        {
            MEOW_WARN("InputAxis was not found in input scheme: \"{}\"", name);
            it = m_axes.emplace(name, std::make_unique<InputAxis>()).first;
        }
        return it->second.get();
    }

    InputAxis* InputScheme::AddAxis(const std::string& name, std::unique_ptr<InputAxis>&& axis)
    {
        return m_axes.emplace(name, std::move(axis)).first->second.get();
    }

    void InputScheme::RemoveAxis(const std::string& name)
    {
        if (auto it = m_axes.find(name); it != m_axes.end())
            m_axes.erase(it);
    }

    InputButton* InputScheme::GetButton(const std::string& name)
    {
        auto it = m_buttons.find(name);
        if (it == m_buttons.end())
        {
            MEOW_WARN("InputButton was not found in input scheme: \"{}\"", name);
            it = m_buttons.emplace(name, std::make_unique<InputButton>()).first;
        }
        return it->second.get();
    }

    InputButton* InputScheme::AddButton(const std::string& name, std::unique_ptr<InputButton>&& button)
    {
        return m_buttons.emplace(name, std::move(button)).first->second.get();
    }

    void InputScheme::RemoveButton(const std::string& name)
    {
        if (auto it = m_buttons.find(name); it != m_buttons.end())
            m_buttons.erase(it);
    }

    void InputScheme::MoveSignals(InputScheme* other)
    {
        if (!other)
            return;
        // Move all axis and button top level signals from the other scheme.
        for (auto& [axis_name, axis] : other->m_axes)
        {
            if (auto it = m_axes.find(axis_name); it != m_axes.end())
                std::swap(it->second->OnAxis(), axis->OnAxis());
            else
                MEOW_WARN("InputAxis was not found in input scheme: \"{}\"", axis_name);
        }
        for (auto& [button_name, button] : other->m_buttons)
        {
            if (auto it = m_buttons.find(button_name); it != m_buttons.end())
                std::swap(it->second->OnButton(), button->OnButton());
            else
                MEOW_WARN("InputButton was not found in input scheme: \"{}\"", button_name);
        }
    }

} // namespace Meow
