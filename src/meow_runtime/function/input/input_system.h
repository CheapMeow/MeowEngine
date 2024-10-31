#pragma once

#include "function/input/axes/mouse_input_axis.h"
#include "function/input/buttons/keyboard_input_button.h"
#include "function/input/buttons/mouse_input_button.h"
#include "function/system.h"
#include "input_scheme.h"

namespace Meow
{
    class Window;

    class InputSystem : public System
    {
    public:
        InputSystem();

        void Start() override;

        void Tick(float dt) override;

        InputScheme* GetScheme() const { return m_current_scheme; }
        InputScheme* GetScheme(const std::string& name) const;
        InputScheme* AddScheme(const std::string& name, std::unique_ptr<InputScheme>&& scheme, bool setCurrent = false);
        void         RemoveScheme(const std::string& name);
        void         SetScheme(InputScheme* scheme);
        void         SetScheme(const std::string& name);

        InputAxis*   GetAxis(const std::string& name) const;
        InputButton* GetButton(const std::string& name) const;

        void BindDefault(std::shared_ptr<Window> window_ptr);

    private:
        std::map<std::string, std::unique_ptr<InputScheme>> schemes;

        std::unique_ptr<InputScheme> m_null_scheme;
        InputScheme*                 m_current_scheme = nullptr;
    };
} // namespace Meow
