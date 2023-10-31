#include "input_system.h"

#include "core/input/buttons/keyboard_input_button.h"
#include "core/input/buttons/mouse_input_button.h"
#include "core/log/log.h"
#include "function/ecs/components/3d/transform/transform_3d_component.h"
#include "function/ecs/components/shared/pawn_component.h"
#include "function/global/runtime_global_context.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Meow
{
    /**
     * @brief Initialize null scheme and current scheme.
     *
     * The reason why we need a null scheme is, user may want to get axis or get button when input scheme is not
     * specified.
     *
     * And, when user bind singal when input scheme is not specified, and then user switch to a new scheme,
     * we want to preserve signals to same name axes or buttons, from the current scheme to the new one.
     *
     * So if you don't have a scheme when input scheme is not specified, you can't store these information.
     *
     * In this case, we name the scheme used when input scheme is not specified as "null scheme".
     */
    InputSystem::InputSystem()
        : m_null_scheme(std::make_unique<InputScheme>())
        , m_current_scheme(m_null_scheme.get())
    {
        // TODO: Support json to get default input scheme
        m_current_scheme->AddButton("Left", std::make_unique<KeyboardInputButton>(KeyCode::A));
        m_current_scheme->AddButton("Right", std::make_unique<KeyboardInputButton>(KeyCode::D));
        m_current_scheme->AddButton("Forward", std::make_unique<KeyboardInputButton>(KeyCode::W));
        m_current_scheme->AddButton("Backward", std::make_unique<KeyboardInputButton>(KeyCode::S));
        m_current_scheme->AddButton("Up", std::make_unique<KeyboardInputButton>(KeyCode::Q));
        m_current_scheme->AddButton("Down", std::make_unique<KeyboardInputButton>(KeyCode::E));

        m_current_scheme->AddButton("Forward", std::make_unique<MouseInputButton>(MouseButtonCode::ButtonLeft));
        m_current_scheme->AddButton("Backward", std::make_unique<MouseInputButton>(MouseButtonCode::ButtonRight));
    }

    void InputSystem::Update(float frame_time)
    {
        for (auto [entity, transform_component, pawn_component] :
             g_runtime_global_context.registry.view<Transform3DComponent, const PawnComponent>().each())
        {
            if (pawn_component.is_player)
            {
                glm::mat4 transform = transform_component.global_transform;

                glm::vec3 right = transform * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                // glm::vec3 up = transform * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
                glm::vec3 forward = transform * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

                glm::vec3 movement = glm::vec3(0.0f);

                if (GetButton("Left")->GetAction() == InputAction::Press)
                {
                    movement += -right;
                }
                if (GetButton("Right")->GetAction() == InputAction::Press)
                {
                    movement += right;
                }
                if (GetButton("Forward")->GetAction() == InputAction::Press)
                {
                    movement += forward;
                }
                if (GetButton("Backward")->GetAction() == InputAction::Press)
                {
                    movement += -forward;
                }
                if (GetButton("Up")->GetAction() == InputAction::Press)
                {
                    movement += glm::vec3(0.0f, 1.0f, 0.0f);
                }
                if (GetButton("Down")->GetAction() == InputAction::Press)
                {
                    movement += -glm::vec3(0.0f, 1.0f, 0.0f);
                }

                movement *= frame_time * 0.001f;

                transform_component.global_transform = glm::translate(transform_component.global_transform, movement);
                transform_component.local_transform  = glm::translate(transform_component.local_transform, movement);
            }
        }
    }

    InputScheme* InputSystem::GetScheme(const std::string& name) const
    {
        auto it = schemes.find(name);
        if (it == schemes.end())
        {
            RUNTIME_ERROR("Could not find input scheme: \"{}\"", name);
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
