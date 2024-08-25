#include "vertex_attribute.h"

namespace Meow
{
    uint32_t VertexAttributeToSize(VertexAttributeBit attribute)
    {
        if (attribute == VertexAttributeBit::Position)
        {
            return 3 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::UV0)
        {
            return 2 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::UV1)
        {
            return 2 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::Normal)
        {
            return 3 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::Tangent)
        {
            return 4 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::Color)
        {
            return 3 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::SkinWeight)
        {
            return 4 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::SkinIndex)
        {
            return 4 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::SkinPack)
        {
            return 3 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::Custom0 || attribute == VertexAttributeBit::Custom1 ||
            attribute == VertexAttributeBit::Custom2 || attribute == VertexAttributeBit::Custom3)
        {
            return 4 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::InstanceFloat1)
        {
            return 1 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::InstanceFloat2)
        {
            return 2 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::InstanceFloat3)
        {
            return 3 * sizeof(float);
        }
        if (attribute == VertexAttributeBit::InstanceFloat4)
        {
            return 4 * sizeof(float);
        }

        return 0;
    }

    uint32_t VertexAttributesToSize(BitMask<VertexAttributeBit> attributes)
    {
        uint32_t size = 0;
        if (attributes & VertexAttributeBit::Position)
        {
            size += 3 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::UV0)
        {
            size += 2 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::UV1)
        {
            size += 2 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::Normal)
        {
            size += 3 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::Tangent)
        {
            size += 4 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::Color)
        {
            size += 3 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::SkinWeight)
        {
            size += 4 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::SkinIndex)
        {
            size += 4 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::SkinPack)
        {
            size += 3 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::Custom0 || attributes & VertexAttributeBit::Custom1 ||
            attributes & VertexAttributeBit::Custom2 || attributes & VertexAttributeBit::Custom3)
        {
            size += 4 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::InstanceFloat1)
        {
            size += 1 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::InstanceFloat2)
        {
            size += 2 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::InstanceFloat3)
        {
            size += 3 * sizeof(float);
        }
        if (attributes & VertexAttributeBit::InstanceFloat4)
        {
            size += 4 * sizeof(float);
        }

        return size;
    }

    vk::Format VertexAttributeToVkFormat(VertexAttributeBit attribute)
    {
        vk::Format format = vk::Format::eR32G32B32Sfloat;
        if (attribute == VertexAttributeBit::Position)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttributeBit::UV0)
        {
            format = vk::Format::eR32G32Sfloat;
        }
        else if (attribute == VertexAttributeBit::UV1)
        {
            format = vk::Format::eR32G32Sfloat;
        }
        else if (attribute == VertexAttributeBit::Normal)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttributeBit::Tangent)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (attribute == VertexAttributeBit::Color)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttributeBit::SkinPack)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttributeBit::SkinWeight)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (attribute == VertexAttributeBit::SkinIndex)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (attribute == VertexAttributeBit::Custom0 || attribute == VertexAttributeBit::Custom1 ||
                 attribute == VertexAttributeBit::Custom2 || attribute == VertexAttributeBit::Custom3)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (attribute == VertexAttributeBit::InstanceFloat1)
        {
            format = vk::Format::eR32Sfloat;
        }
        else if (attribute == VertexAttributeBit::InstanceFloat2)
        {
            format = vk::Format::eR32G32Sfloat;
        }
        else if (attribute == VertexAttributeBit::InstanceFloat3)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttributeBit::InstanceFloat4)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }

        return format;
    }

    VertexAttributeBit StringToVertexAttribute(const std::string& name)
    {
        if (name == "inPosition")
        {
            return VertexAttributeBit::Position;
        }
        else if (name == "inUV0")
        {
            return VertexAttributeBit::UV0;
        }
        else if (name == "inUV1")
        {
            return VertexAttributeBit::UV1;
        }
        else if (name == "inNormal")
        {
            return VertexAttributeBit::Normal;
        }
        else if (name == "inTangent")
        {
            return VertexAttributeBit::Tangent;
        }
        else if (name == "inColor")
        {
            return VertexAttributeBit::Color;
        }
        else if (name == "inSkinWeight")
        {
            return VertexAttributeBit::SkinWeight;
        }
        else if (name == "inSkinIndex")
        {
            return VertexAttributeBit::SkinIndex;
        }
        else if (name == "inSkinPack")
        {
            return VertexAttributeBit::SkinPack;
        }
        else if (name == "inCustom0")
        {
            return VertexAttributeBit::Custom0;
        }
        else if (name == "inCustom1")
        {
            return VertexAttributeBit::Custom1;
        }
        else if (name == "inCustom2")
        {
            return VertexAttributeBit::Custom2;
        }
        else if (name == "inCustom3")
        {
            return VertexAttributeBit::Custom3;
        }

        return VertexAttributeBit::None;
    }

    const std::string VertexAttributeToString(VertexAttributeBit bit)
    {
        switch (bit)
        {
            case VertexAttributeBit::Position:
                return "inPosition";
            case VertexAttributeBit::UV0:
                return "inUV0";
            case VertexAttributeBit::UV1:
                return "inUV1";
            case VertexAttributeBit::Normal:
                return "inNormal";
            case VertexAttributeBit::Tangent:
                return "inTangent";
            case VertexAttributeBit::Color:
                return "inColor";
            case VertexAttributeBit::SkinWeight:
                return "inSkinWeight";
            case VertexAttributeBit::SkinIndex:
                return "inSkinIndex";
            case VertexAttributeBit::SkinPack:
                return "inSkinPack";
            case VertexAttributeBit::Custom0:
                return "inCustom0";
            case VertexAttributeBit::Custom1:
                return "inCustom1";
            case VertexAttributeBit::Custom2:
                return "inCustom2";
            case VertexAttributeBit::Custom3:
                return "inCustom3";
            case VertexAttributeBit::None:
                return "None";
            default:
                return "Unknown";
        }
    }
} // namespace Meow
