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

    uint32_t VertexAttributesToSize(std::vector<VertexAttributeBit> attributes)
    {
        uint32_t size = 0;
        for (size_t i = 0; i < attributes.size(); ++i)
        {
            size += VertexAttributeToSize(attributes[i]);
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
} // namespace Meow
