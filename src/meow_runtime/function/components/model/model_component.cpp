#include "model_component.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    ModelComponent::ModelComponent(std::vector<float>&&        vertices,
                                   std::vector<uint32_t>&&     indices,
                                   BitMask<VertexAttributeBit> attributes)
    {
        auto [success, model_uuid] =
            g_runtime_context.resource_system->LoadModel(std::move(vertices), std::move(indices), attributes);
        if (success)
            model_ptr = g_runtime_context.resource_system->GetModel(model_uuid);
    }

    ModelComponent::ModelComponent(const std::string& file_path, BitMask<VertexAttributeBit> attributes)
    {
        auto [success, model_uuid] = g_runtime_context.resource_system->LoadModel(file_path, attributes);
        if (success)
            model_ptr = g_runtime_context.resource_system->GetModel(model_uuid);
    }
} // namespace Meow