#pragma once

#include "core/base/bitmask.hpp"
#include "function/global/runtime_global_context.h"
#include "function/object/game_object.h"
#include "function/render/structs/model.h"

#include <vector>

namespace Meow
{
    class [[reflectable_class()]] ModelComponent : public Component
    {
    public:
        std::weak_ptr<Model> model_ptr;

        ModelComponent(const std::string& file_path, BitMask<VertexAttributeBit> attributes)
        {
            model_ptr = g_runtime_global_context.resource_system->LoadModel(file_path, attributes);
        }
    };
} // namespace Meow