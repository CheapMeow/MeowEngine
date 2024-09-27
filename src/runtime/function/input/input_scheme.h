#pragma once

#include "pch.h"

#include "core/base/non_copyable.h"
#include "function/window/window.h"
#include "input_axis.h"
#include "input_button.h"

#include <map>
#include <memory>
#include <string>

namespace Meow
{
    struct InputScheme : public NonCopyable
    {
        std::map<std::string, InputAxis>   axes;
        std::map<std::string, InputButton> buttons;
    };
} // namespace Meow
