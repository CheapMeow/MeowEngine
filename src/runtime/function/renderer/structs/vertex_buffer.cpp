#include "vertex_buffer.h"

namespace Meow
{
    /**
     * @brief Create vector of VkVertexInputAttributeDescription according to vertex attributes.
     *
     * Provide vector of VkVertexInputAttributeDescription for pipeline creation.
     */
    std::vector<VkVertexInputAttributeDescription> VertexBuffer::GetInputAttributes() const
    {
        std::vector<VkVertexInputAttributeDescription> vertex_input_attributes;
        int32_t                                        offset = 0;
        for (int32_t i = 0; i < attributes.size(); ++i)
        {
            VkVertexInputAttributeDescription vertex_input_attribute = {};
            vertex_input_attribute.binding                           = 0;
            vertex_input_attribute.location                          = i;
            vertex_input_attribute.format                            = VertexAttributeToVkFormat(attributes[i]);
            vertex_input_attribute.offset                            = offset;
            offset += VertexAttributeToSize(attributes[i]);
            vertex_input_attributes.push_back(vertex_input_attribute);
        }
        return vertex_input_attributes;
    }

    /**
     * @brief Create vector of VkVertexInputBindingDescription according to vertex attributes.
     *
     * Provide vector of VkVertexInputBindingDescription for pipeline creation.
     */
    VkVertexInputBindingDescription VertexBuffer::GetInputBinding() const
    {
        int32_t stride = 0;
        for (int32_t i = 0; i < attributes.size(); ++i)
        {
            stride += VertexAttributeToSize(attributes[i]);
        }

        VkVertexInputBindingDescription vertex_input_binding = {};
        vertex_input_binding.binding                         = 0;
        vertex_input_binding.stride                          = stride;
        vertex_input_binding.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

        return vertex_input_binding;
    }

    void VertexBuffer::Bind(const vk::raii::CommandBuffer& cmd_buffer) const
    {
        cmd_buffer.bindVertexBuffers(0, {*buffer_data.buffer}, {0});
    }
} // namespace Meow