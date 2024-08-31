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
        UUIDv4::UUID         uuid;
        std::weak_ptr<Model> model_ptr;

        ModelComponent(std::vector<float>&&        vertices,
                       std::vector<uint32_t>&&     indices,
                       BitMask<VertexAttributeBit> attributes)
        {
            if (g_runtime_global_context.resource_system->LoadModel(
                    std::move(vertices), std::move(indices), attributes, uuid))
                model_ptr = g_runtime_global_context.resource_system->GetModel(uuid);
        }

        ModelComponent(const std::string& file_path, BitMask<VertexAttributeBit> attributes)
        {
            if (g_runtime_global_context.resource_system->LoadModel(file_path, attributes, uuid))
                model_ptr = g_runtime_global_context.resource_system->GetModel(uuid);
        }
    };
} // namespace Meow