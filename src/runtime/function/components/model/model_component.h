#pragma once

#include "function/components/component.h"
#include "function/global/runtime_global_context.h"
#include "function/render/structs/model.h"

#include <vector>

namespace Meow
{
    class [[reflectable_class()]] ModelComponent : public Component
    {
    public:
        std::weak_ptr<Model> model_ptr;

        ModelComponent(const std::string&           file_path,
                       std::vector<VertexAttribute> attributes,
                       vk::IndexType                index_type = vk::IndexType::eUint16)
        {
            model_ptr = g_runtime_global_context.render_system->CreateModelFromFile(file_path, attributes, index_type);
        }
    };
} // namespace Meow