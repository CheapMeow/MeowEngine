#pragma once

#include "core/base/non_copyable.h"
#include "function/render/buffer_data/uniform_buffer.h"
#include "function/resource/resource_base.h"
#include "shader.h"
#include "shading_model_type.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Meow
{
    class Material : public ResourceBase
    {
    public:
        friend class MaterialFactory;

        Material(std::nullptr_t) {}

        Material(std::shared_ptr<Shader> shader);

        Material(Material&& rhs) noexcept { swap(*this, rhs); }

        Material& operator=(Material&& rhs) noexcept
        {
            if (this != &rhs)
            {
                swap(*this, rhs);
            }

            return *this;
        }

        std::shared_ptr<Shader> GetShader() { return shader; }

        void BindPipeline(const vk::raii::CommandBuffer& command_buffer);

        void BindBufferToDescriptorSet(const std::string&          name,
                                       const vk::raii::Buffer&     buffer,
                                       vk::DeviceSize              range            = VK_WHOLE_SIZE,
                                       const vk::raii::BufferView* raii_buffer_view = nullptr,
                                       uint32_t                    frame_index      = 0);

        void BindImageToDescriptorSet(const std::string& name, ImageData& image_data, uint32_t frame_index = 0);

        void BeginPopulatingDynamicUniformBufferPerFrame();

        void EndPopulatingDynamicUniformBufferPerFrame();

        void BeginPopulatingDynamicUniformBufferPerObject();

        void EndPopulatingDynamicUniformBufferPerObject();

        void PopulateDynamicUniformBuffer(const std::string& name, void* data, uint32_t size);

        void PopulateUniformBuffer(const std::string& name, void* data, uint32_t size);

        void BindDescriptorSetToPipeline(const vk::raii::CommandBuffer& command_buffer,
                                         uint32_t                       first_set,
                                         uint32_t                       set_count,
                                         uint32_t                       draw_call  = 0,
                                         bool                           is_dynamic = false);

        ShadingModelType GetShadingModelType() { return m_shading_model_type; }

        void SetDebugName(const std::string& debug_name);

        friend void swap(Material& lhs, Material& rhs);

        std::shared_ptr<Shader> shader = nullptr;

    private:
        void CreateUniformBuffer();

        vk::raii::Pipeline graphics_pipeline = nullptr;
        vk::raii::Pipeline compute_pipeline  = nullptr;

        // stored for binding descriptor set

        bool                                                            m_actived   = false;
        int32_t                                                         m_obj_count = 0;
        std::vector<std::vector<uint32_t>>                              m_per_obj_dynamic_offsets;
        vk::raii::DescriptorSets                                        m_descriptor_sets = nullptr;
        std::unordered_map<std::string, std::unique_ptr<UniformBuffer>> m_uniform_buffers;
        std::unique_ptr<UniformBuffer>                                  m_dynamic_uniform_buffer;

        ShadingModelType m_shading_model_type;
    };
} // namespace Meow
