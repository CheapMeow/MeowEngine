#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    enum VertexAttribute
    {
        VA_None = 0,
        VA_Position,
        VA_UV0,
        VA_UV1,
        VA_Normal,
        VA_Tangent,
        VA_Color,
        VA_SkinWeight,
        VA_SkinIndex,
        VA_SkinPack,
        VA_InstanceFloat1,
        VA_InstanceFloat2,
        VA_InstanceFloat3,
        VA_InstanceFloat4,
        VA_Custom0,
        VA_Custom1,
        VA_Custom2,
        VA_Custom3,
        VA_Count,
    };

    uint32_t VertexAttributeToSize(VertexAttribute attribute);

    uint32_t VertexAttributesToSize(const std::vector<VertexAttribute>& vertex_attributes);

    vk::Format VertexAttributeToVkFormat(VertexAttribute attribute);

    VertexAttribute StringToVertexAttribute(const std::string& name);

} // namespace Meow