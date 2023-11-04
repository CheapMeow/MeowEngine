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

    VkFormat VertexAttributeToVkFormat(VertexAttribute attribute)
    {
        VkFormat format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
        if (attribute == VertexAttribute::VA_Position)
        {
            format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_UV0)
        {
            format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_UV1)
        {
            format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_Normal)
        {
            format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_Tangent)
        {
            format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_Color)
        {
            format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_SkinPack)
        {
            format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_SkinWeight)
        {
            format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_SkinIndex)
        {
            format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_Custom0 || attribute == VertexAttribute::VA_Custom1 ||
                 attribute == VertexAttribute::VA_Custom2 || attribute == VertexAttribute::VA_Custom3)
        {
            format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat1)
        {
            format = VkFormat::VK_FORMAT_R32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat2)
        {
            format = VkFormat::VK_FORMAT_R32G32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat3)
        {
            format = VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
        }
        else if (attribute == VertexAttribute::VA_InstanceFloat4)
        {
            format = VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
        }

        return format;
    }
} // namespace Meow
