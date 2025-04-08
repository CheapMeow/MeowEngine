#include "shader.h"

namespace Meow
{
    Shader::~Shader()
    {
        if (m_dispatcher)
        {
            for (size_t i = 0; i < descriptor_set_layouts.size(); ++i)
            {
                m_dispatcher->vkDestroyDescriptorSetLayout(
                    static_cast<VkDevice>(m_device),
                    static_cast<VkDescriptorSetLayout>(descriptor_set_layouts[i]),
                    nullptr);
            }
        }
    }
} // namespace Meow