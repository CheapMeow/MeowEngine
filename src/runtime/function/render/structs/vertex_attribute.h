#pragma once

#include "core/base/bitmask.hpp"
#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    enum class VertexAttributeBit : uint32_t
    {
        None           = 0x00000000,
        Position       = 0x00000001,
        UV0            = 0x00000002,
        UV1            = 0x00000004,
        Normal         = 0x00000008,
        Tangent        = 0x00000010,
        Color          = 0x00000020,
        SkinWeight     = 0x00000040,
        SkinIndex      = 0x00000080,
        SkinPack       = 0x00000100,
        InstanceFloat1 = 0x00000200,
        InstanceFloat2 = 0x00000400,
        InstanceFloat3 = 0x00000800,
        InstanceFloat4 = 0x00001000,
        Custom0        = 0x00002000,
        Custom1        = 0x00004000,
        Custom2        = 0x00008000,
        Custom3        = 0x00010000,
        ALL            = 0x0001FFFF,
    };

    uint32_t           VertexAttributeToSize(VertexAttributeBit attribute);
    uint32_t           VertexAttributesToSize(BitMask<VertexAttributeBit> attributes);
    vk::Format         VertexAttributeToVkFormat(VertexAttributeBit attribute);
    VertexAttributeBit StringToVertexAttribute(const std::string& name);
    const std::string  VertexAttributeToString(VertexAttributeBit attribute);
} // namespace Meow