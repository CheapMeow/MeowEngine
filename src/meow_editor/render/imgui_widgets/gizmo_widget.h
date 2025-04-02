#pragma once

#include "meow_runtime/function/object/game_object.h"

namespace Meow
{
    class GizmoWidget
    {
    public:
        void ShowGameObjectGizmo(UUID id);

    private:
        int m_GizmoType = -1;
    };
} // namespace Meow