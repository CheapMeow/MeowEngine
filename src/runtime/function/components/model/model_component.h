#pragma once

#include "core/base/bitmask.hpp"
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

        [[reflectable_field()]]
        std::vector<std::string> m_image_paths;

        ModelComponent(std::vector<float>&&        vertices,
                       std::vector<uint32_t>&&     indices,
                       BitMask<VertexAttributeBit> attributes);

        ModelComponent(const std::string& file_path, BitMask<VertexAttributeBit> attributes);
    };
} // namespace Meow