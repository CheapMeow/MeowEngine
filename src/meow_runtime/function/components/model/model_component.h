#pragma once

#include "function/object/game_object.h"
#include "function/render/model/model.hpp"

#include <vector>

namespace Meow
{
    class [[reflectable_class()]] ModelComponent : public Component
    {
    public:
        UUID                 uuid;
        std::weak_ptr<Model> model;
        UUID                 material_id;
    };
} // namespace Meow