#pragma once

#include "buffer_data.h"
#include "core/base/alignment.h"
#include "shader.h"

#include <limits>
#include <memory>
#include <tuple>
#include <vector>

namespace Meow
{
    struct RingUniformBuffer
    {
        std::shared_ptr<BufferData> buffer_data_ptr  = nullptr;
        void*                       mapped_data_ptr  = nullptr;
        uint64_t                    buffer_size      = 0;
        uint64_t                    allocated_memory = 0;
        uint32_t                    min_alignment    = 0;

        RingUniformBuffer(std::nullptr_t) {}

        RingUniformBuffer(vk::raii::PhysicalDevice const& physical_device, vk::raii::Device const& logical_device)
        {
            buffer_size   = 32 * 1024; // 32KB
            min_alignment = physical_device.getProperties().limits.minUniformBufferOffsetAlignment;

            buffer_data_ptr = std::make_shared<BufferData>(physical_device,
                                                           logical_device,
                                                           buffer_size,
                                                           vk::BufferUsageFlagBits::eUniformBuffer |
                                                               vk::BufferUsageFlagBits::eTransferDst);
            mapped_data_ptr = buffer_data_ptr->device_memory.mapMemory(0, VK_WHOLE_SIZE);
        }

        ~RingUniformBuffer()
        {
            mapped_data_ptr = nullptr;
            if (buffer_data_ptr != nullptr)
                buffer_data_ptr->device_memory.unmapMemory();
            buffer_data_ptr = nullptr;
        }

        uint64_t AllocateMemory(uint64_t size)
        {
            uint64_t new_memory_start = Align<uint64_t>(allocated_memory, min_alignment);

            if (new_memory_start + size <= buffer_size)
            {
                allocated_memory = new_memory_start + size;
                return new_memory_start;
            }

            // TODO: Scalable buffer size
            // When the memory is running out, allocation return to the begin of memory
            // It means overriding the old buffer data
            // If the object number is very large, it definitely break the uniform data at this frame
            allocated_memory = size;
            return 0;
        }
    };

    /**
     * @brief Information about uniform buffer that are same for all objects.
     *
     * Because it will be copy to new allocation memoty of ring buffer when drawing each new object,
     * the data should be stored itself.
     */
    struct GlobalUniformBufferInfo
    {
        uint32_t             dynamic_offset_index = 0;
        std::vector<uint8_t> data;
        uint32_t             dynamic_offset = std::numeric_limits<uint32_t>::max();
    };

    class Material
    {
    public:
        Material(std::nullptr_t) {}

        Material(vk::raii::PhysicalDevice const& physical_device,
                 vk::raii::Device const&         logical_device,
                 std::shared_ptr<Shader>         shader_ptr);

        void CreatePipeline(vk::raii::Device const&     logical_device,
                            vk::raii::RenderPass const& render_pass,
                            vk::FrontFace               front_face,
                            bool                        depth_buffered);

        void BeginFrame();

        void EndFrame();

        void BeginObject();

        void EndObject();

        void SetGlobalUniformBuffer(const std::string& name, void* dataPtr, uint32_t size);

        void SetLocalUniformBuffer(const std::string& name, void* dataPtr, uint32_t size);

        void SetStorageBuffer(vk::raii::Device const&     logical_device,
                              const std::string&          name,
                              vk::raii::Buffer const&     buffer,
                              vk::DeviceSize              range            = VK_WHOLE_SIZE,
                              vk::raii::BufferView const* raii_buffer_view = nullptr);

        void SetImage(vk::raii::Device const& logical_device, const std::string& name, TextureData& texture_data);

        void BindPipeline(vk::raii::CommandBuffer const& command_buffer);

        void BindDescriptorSets(vk::raii::CommandBuffer const& command_buffer, int32_t obj_index);

    public:
        std::shared_ptr<Shader> shader_ptr             = nullptr;
        int                     color_attachment_count = 1;
        int                     subpass                = 0;

    private:
        RingUniformBuffer ring_buffer = nullptr;

        vk::raii::Pipeline graphics_pipeline = nullptr;

        // stored for binding descriptor set

        bool                                 actived   = false;
        int32_t                              obj_count = 0;
        std::vector<GlobalUniformBufferInfo> global_uniform_buffer_infos;
        std::vector<std::vector<uint32_t>>   per_obj_dynamic_offsets;
        std::vector<vk::DescriptorSet>       descriptor_sets;
    };

} // namespace Meow
