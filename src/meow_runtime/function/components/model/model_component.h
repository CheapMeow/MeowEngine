#pragma once

#include "function/object/game_object.h"
#include "function/render/render_resources/model.hpp"

#include <vector>

namespace Meow
{
    class [[reflectable_class()]] ModelComponent : public Component
    {
    public:
        UUID                 uuid;
        std::weak_ptr<Model> model_ptr;
    };
} // namespace Meow