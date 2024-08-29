#include "input_system.h"

#include "function/components/shared/pawn_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_global_context.h"
#include "function/input/axes/mouse_input_axis.h"
#include "function/input/buttons/keyboard_input_button.h"
#include "function/input/buttons/mouse_input_button.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    /**
     * @brief Initialize null scheme and current scheme.
     *
     * The reason why we need a null scheme is, user may want to get axis or get button when input scheme is not
     * specified.
     *
     * And, when user bind signal when input scheme is not specified, and then user switch to a new scheme,
     * we want to preserve signals to same name axes or buttons, from the current scheme to the new one.
     *
     * So if you don't have a scheme when input scheme is not specified, you can't store these information.
     *
     * In this case, we name the scheme used when input scheme is not specified as "null scheme".
     */
    InputSystem::InputSystem()
        : m_null_scheme(std::make_unique<InputScheme>())
        , m_current_scheme(m_null_scheme.get())
    {}

    void InputSystem::Start()
    {
        // TODO: Support json to get default input scheme
        m_current_scheme->AddButton("Left", std::make_unique<KeyboardInputButton>(KeyCode::A));
        m_current_scheme->AddButton("Right", std::make_unique<KeyboardInputButton>(KeyCode::D));
        m_current_scheme->AddButton("Forward", std::make_unique<KeyboardInputButton>(KeyCode::W));
        m_current_scheme->AddButton("Backward", std::make_unique<KeyboardInputButton>(KeyCode::S));
        m_current_scheme->AddButton("Up", std::make_unique<KeyboardInputButton>(KeyCode::E));
        m_current_scheme->AddButton("Down", std::make_unique<KeyboardInputButton>(KeyCode::Q));

        m_current_scheme->AddButton("LeftMouse", std::make_unique<MouseInputButton>(MouseButtonCode::ButtonLeft));
        m_current_scheme->AddButton("RightMouse", std::make_unique<MouseInputButton>(MouseButtonCode::ButtonRight));

        m_current_scheme->AddAxis("MouseX", std::make_unique<MouseInputAxis>(0));
        m_current_scheme->AddAxis("MouseY", std::make_unique<MouseInputAxis>(1));
    }

    void InputSystem::Tick(float dt) {}

    InputScheme* InputSystem::GetScheme(const std::string& name) const
    {
        auto it = schemes.find(name);
        if (it == schemes.end())
        {
            MEOW_ERROR("Could not find input scheme: \"{}\"", name);
            return nullptr;
        }
        return it->second.get();
    }

    InputScheme*
    InputSystem::AddScheme(const std::string& name, std::unique_ptr<InputScheme>&& scheme, bool set_current)
    {
        auto inputScheme = schemes.emplace(name, std::move(scheme)).first->second.get();
        if (!m_current_scheme || set_current)
            SetScheme(inputScheme);
        return inputScheme;
    }

    void InputSystem::RemoveScheme(const std::string& name)
    {
        auto it = schemes.find(name);
        if (m_current_scheme == it->second.get())
            SetScheme(nullptr);
        if (it != schemes.end())
            schemes.erase(it);
        // If we have no current scheme grab some random one from the map.
        if (!m_current_scheme && !schemes.empty())
            m_current_scheme = schemes.begin()->second.get();
    }

    void InputSystem::SetScheme(InputScheme* scheme)
    {
        if (!scheme)
            scheme = m_null_scheme.get();
        // We want to preserve signals from the current scheme to the new one.
        scheme->MoveSignals(m_current_scheme);
        m_current_scheme = scheme;
    }

    void InputSystem::SetScheme(const std::string& name)
    {
        auto scheme = GetScheme(name);
        if (!scheme)
            return;
        SetScheme(scheme);
    }

    InputAxis* InputSystem::GetAxis(const std::string& name) const
    {
        if (m_current_scheme)
            return m_current_scheme->GetAxis(name);
        return nullptr;
    }

    InputButton* InputSystem::GetButton(const std::string& name) const
    {
        if (m_current_scheme)
            return m_current_scheme->GetButton(name);
        return nullptr;
    }
} // namespace Meow
