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
        std::map<std::string, std::unique_ptr<InputAxis>>   axes;
        std::map<std::string, std::unique_ptr<InputButton>> buttons;
    };
} // namespace Meow
