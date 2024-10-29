#include "input_system.h"

#include "function/components/shared/pawn_component.h"
#include "function/components/transform/transform_3d_component.hpp"
#include "function/global/runtime_global_context.h"
#include "runtime.h"

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

    void InputSystem::Start() {}

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
        if (!m_current_scheme)
            return nullptr;

        const auto it = m_current_scheme->axes.find(name);
        if (it != m_current_scheme->axes.end())
            return (*it).second.get();
        else
            return nullptr;
    }

    InputButton* InputSystem::GetButton(const std::string& name) const
    {
        if (!m_current_scheme)
            return nullptr;

        const auto it = m_current_scheme->buttons.find(name);
        if (it != m_current_scheme->buttons.end())
            return (*it).second.get();
        else
            return nullptr;
    }

    void InputSystem::BindDefault(std::shared_ptr<Window> window_ptr)
    {
        // TODO: Support json to get default input scheme
        m_current_scheme->buttons["Left"]     = std::make_unique<KeyboardInputButton>(window_ptr, KeyCode::A);
        m_current_scheme->buttons["Right"]    = std::make_unique<KeyboardInputButton>(window_ptr, KeyCode::D);
        m_current_scheme->buttons["Forward"]  = std::make_unique<KeyboardInputButton>(window_ptr, KeyCode::W);
        m_current_scheme->buttons["Backward"] = std::make_unique<KeyboardInputButton>(window_ptr, KeyCode::S);
        m_current_scheme->buttons["Up"]       = std::make_unique<KeyboardInputButton>(window_ptr, KeyCode::E);
        m_current_scheme->buttons["Down"]     = std::make_unique<KeyboardInputButton>(window_ptr, KeyCode::Q);

        m_current_scheme->buttons["LeftMouse"] =
            std::make_unique<MouseInputButton>(window_ptr, MouseButtonCode::ButtonLeft);
        m_current_scheme->buttons["RightMouse"] =
            std::make_unique<MouseInputButton>(window_ptr, MouseButtonCode::ButtonRight);

        m_current_scheme->axes["MouseX"] = std::make_unique<MouseInputAxis>(window_ptr, 0);
        m_current_scheme->axes["MouseY"] = std::make_unique<MouseInputAxis>(window_ptr, 1);

        window_ptr->OnClose().connect([&]() { MeowRuntime::Get().SetRunning(false); });
    }
} // namespace Meow
