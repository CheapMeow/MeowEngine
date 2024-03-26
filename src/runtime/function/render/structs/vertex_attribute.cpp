#include "vertex_attribute.h"

namespace Meow
{
    int32_t VertexAttributeToSize(VertexAttribute attribute)
    {
        // count * sizeof(float)
        if (attribute == VertexAttribute::VA_Position)
        {
            return 3 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_UV0)
        {
            return 2 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_UV1)
        {
            return 2 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_Normal)
        {
            return 3 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_Tangent)
        {
            return 4 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_Color)
        {
            return 3 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_SkinWeight)
        {
            return 4 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_SkinIndex)
        {
            return 4 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_SkinPack)
        {
            return 3 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_Custom0 || attribute == VertexAttribute::VA_Custom1 ||
                 attribute == VertexAttribute::VA_Custom2 || attribute == VertexAttribute::VA_Custom3)
        {
            return 4 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat1)
        {
            return 1 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat2)
        {
            return 2 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat3)
        {
            return 3 * sizeof(float);
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat4)
        {
            return 4 * sizeof(float);
        }

        return 0;
    }

    int32_t VertexAttributesToSize(const std::vector<VertexAttribute>& vertex_attributes)
    {
        int32_t size = 0;
        for (auto attribute : vertex_attributes)
        {
            size += VertexAttributeToSize(attribute);
        }

        assert(size > 0);

        return size;
    }

    vk::Format VertexAttributeToVkFormat(VertexAttribute attribute)
    {
        vk::Format format = vk::Format::eR32G32B32Sfloat;
        if (attribute == VertexAttribute::VA_Position)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_UV0)
        {
            format = vk::Format::eR32G32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_UV1)
        {
            format = vk::Format::eR32G32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_Normal)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_Tangent)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_Color)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_SkinPack)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_SkinWeight)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_SkinIndex)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_Custom0 || attribute == VertexAttribute::VA_Custom1 ||
                 attribute == VertexAttribute::VA_Custom2 || attribute == VertexAttribute::VA_Custom3)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat1)
        {
            format = vk::Format::eR32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat2)
        {
            format = vk::Format::eR32G32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat3)
        {
            format = vk::Format::eR32G32B32Sfloat;
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat4)
        {
            format = vk::Format::eR32G32B32A32Sfloat;
        }

        return format;
    }

    VertexAttribute StringToVertexAttribute(const std::string& name)
    {
        if (name == "inPosition")
        {
            return VertexAttribute::VA_Position;
        }
        else if (name == "inUV0")
        {
            return VertexAttribute::VA_UV0;
        }
        else if (name == "inUV1")
        {
            return VertexAttribute::VA_UV1;
        }
        else if (name == "inNormal")
        {
            return VertexAttribute::VA_Normal;
        }
        else if (name == "inTangent")
        {
            return VertexAttribute::VA_Tangent;
        }
        else if (name == "inColor")
        {
            return VertexAttribute::VA_Color;
        }
        else if (name == "inSkinWeight")
        {
            return VertexAttribute::VA_SkinWeight;
        }
        else if (name == "inSkinIndex")
        {
            return VertexAttribute::VA_SkinIndex;
        }
        else if (name == "inSkinPack")
        {
            return VertexAttribute::VA_SkinPack;
        }
        else if (name == "inCustom0")
        {
            return VertexAttribute::VA_Custom0;
        }
        else if (name == "inCustom1")
        {
            return VertexAttribute::VA_Custom1;
        }
        else if (name == "inCustom2")
        {
            return VertexAttribute::VA_Custom2;
        }
        else if (name == "inCustom3")
        {
            return VertexAttribute::VA_Custom3;
        }

        return VertexAttribute::VA_None;
    }
} // namespace Meow
