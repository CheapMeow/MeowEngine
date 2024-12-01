#pragma once

#include "core/base/non_copyable.h"
#include "function/render/render_resources/uniform_buffer.h"
#include "shader.h"
#ifdef MEOW_DEBUG
#    include "material_stat.h"
#endif

#include <memory>
#include <unordered_map>
#include <vector>

namespace Meow
{
    class Material : NonCopyable
    {
    public:
        Material(std::nullptr_t) {}

        Material(std::shared_ptr<Shader> shader_ptr);

        Material(Material&& rhs) noexcept { swap(*this, rhs); }

        Material& operator=(Material&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }

            return *this;
        }

        void CreatePipeline(vk::FrontFace front_face = vk::FrontFace::eClockwise, bool depth_buffered = false);

        std::shared_ptr<Shader> GetShader() { return shader_ptr; }

        void BindPipeline(const vk::raii::CommandBuffer& command_buffer);

        void BindBufferToDescriptorSet(const std::string&          name,
                                       const vk::raii::Buffer&     buffer,
                                       vk::DeviceSize              range            = VK_WHOLE_SIZE,
                                       const vk::raii::BufferView* raii_buffer_view = nullptr);

        void BindImageToDescriptorSet(const std::string& name, ImageData& image_data);

        void BeginPopulatingDynamicUniformBufferPerFrame();

        void EndPopulatingDynamicUniformBufferPerFrame();

        void BeginPopulatingDynamicUniformBufferPerObject();

        void EndPopulatingDynamicUniformBufferPerObject();

        void PopulateDynamicUniformBuffer(const std::string& name, void* data, uint32_t size);

        void PopulateUniformBuffer(const std::string& name, void* data, uint32_t size);

        void BindDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer,
                                         uint32_t                       first_set,
                                         uint32_t                       set_count,
                                         bool                           is_dynamic = false);

#ifdef MEOW_DEBUG
        MaterialStat GetStat()
        {
            return {m_draw_call,
                    shader_ptr->vertex_attribute_metas,
                    shader_ptr->buffer_meta_map,
                    shader_ptr->image_meta_map};
        }
#endif

        friend void swap(Material& lhs, Material& rhs);

        std::shared_ptr<Shader> shader_ptr             = nullptr;
        int                     color_attachment_count = 1;
        int                     subpass                = 0;

    private:
        void CreateUniformBuffer();

        vk::raii::Pipeline graphics_pipeline = nullptr;

        // stored for binding descriptor set

        bool                                                            m_actived                     = false;
        uint32_t                                                        m_dynamic_uniform_update_call = 0;
        uint32_t                                                        m_draw_call                   = 0;
        std::vector<std::vector<uint32_t>>                              m_per_obj_dynamic_offsets;
        vk::raii::DescriptorSets                                        m_descriptor_sets = nullptr;
        std::unordered_map<std::string, std::unique_ptr<UniformBuffer>> m_uniform_buffers;
        std::unique_ptr<UniformBuffer>                                  m_dynamic_uniform_buffer;
    };
} // namespace Meow
