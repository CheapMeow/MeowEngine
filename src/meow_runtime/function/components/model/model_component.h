#pragma once

#include "core/reflect/macros.h"
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

        [[reflectable_method()]]
        void foo1()
        {
            std::cout << "uuid = " << uuid << std::endl;
        }

        [[reflectable_method()]]
        void foo2() override
        {
            std::cout << "derived class uuid = " << uuid << std::endl;
        }
    };
} // namespace Meow