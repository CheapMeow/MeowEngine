#pragma once

#include "function/system.h"
#include "input_scheme.h"

namespace Meow
{
    class InputSystem : System
    {
    public:
        InputSystem();

        void Start() override;

        void Update(float frame_time);

        InputScheme* GetScheme() const { return m_current_scheme; }
        InputScheme* GetScheme(const std::string& name) const;
        InputScheme* AddScheme(const std::string& name, std::unique_ptr<InputScheme>&& scheme, bool setCurrent = false);
        void         RemoveScheme(const std::string& name);
        void         SetScheme(InputScheme* scheme);
        void         SetScheme(const std::string& name);

        InputAxis*   GetAxis(const std::string& name) const;
        InputButton* GetButton(const std::string& name) const;

    private:
        std::map<std::string, std::unique_ptr<InputScheme>> schemes;

        std::unique_ptr<InputScheme> m_null_scheme;
        InputScheme*                 m_current_scheme = nullptr;
    };
} // namespace Meow
