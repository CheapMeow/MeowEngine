#include "model_component.h"

#include "function/global/runtime_global_context.h"

namespace Meow
{
    ModelComponent::ModelComponent(std::vector<float>&&        vertices,
                                   std::vector<uint32_t>&&     indices,
                                   BitMask<VertexAttributeBit> attributes)
    {
        if (g_runtime_global_context.resource_system->LoadModel(
                std::move(vertices), std::move(indices), attributes, uuid))
            model_ptr = g_runtime_global_context.resource_system->GetModel(uuid);
    }

    ModelComponent::ModelComponent(const std::string& file_path, BitMask<VertexAttributeBit> attributes)
    {
        if (g_runtime_global_context.resource_system->LoadModel(file_path, attributes, uuid))
            model_ptr = g_runtime_global_context.resource_system->GetModel(uuid);
    }
} // namespace Meow