#pragma once

#include "core/base/non_copyable.h"
#include "input_axis.h"
#include "input_button.h"

#include <map>
#include <memory>
#include <string>

namespace Meow
{
    class InputScheme : NonCopyable
    {
        friend class InputSystem;

    public:
        // TODO: Support joystick
        using AxisMap   = std::map<std::string, std::unique_ptr<InputAxis>>;
        using ButtonMap = std::map<std::string, std::unique_ptr<InputButton>>;

        // TODO: Support json file save and load

        InputAxis* GetAxis(const std::string& name);
        InputAxis* AddAxis(const std::string& name, std::unique_ptr<InputAxis>&& axis);
        void       RemoveAxis(const std::string& name);

        InputButton* GetButton(const std::string& name);
        InputButton* AddButton(const std::string& name, std::unique_ptr<InputButton>&& button);
        void         RemoveButton(const std::string& name);

    private:
        void MoveSignals(InputScheme* other);

        AxisMap   m_axes;
        ButtonMap m_buttons;
    };
} // namespace Meow
