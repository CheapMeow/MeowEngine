#pragma once

#include "profile/profile_system.h"

#include <memory>

namespace Meow
{
    struct EditorGlobalContext
    {
        bool running = true;

        std::shared_ptr<ProfileSystem>  profile_system  = nullptr;
    };

    extern EditorGlobalContext g_editor_global_context;
} // namespace Meow