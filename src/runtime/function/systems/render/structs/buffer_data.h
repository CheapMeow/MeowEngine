#pragma once

#include "function/systems/render/utils/vulkan_initialize_utils.hpp"

namespace Meow
{
    struct BufferData
    {
        // the DeviceMemory should be destroyed before the Buffer it is bound to; to get that order with the
        // standard destructor of the BufferData, the order of DeviceMemory and Buffer here matters
        vk::raii::DeviceMemory device_memory = nullptr;
        vk::raii::Buffer       buffer        = nullptr;

        vk::DeviceSize          device_size;
        vk::BufferUsageFlags    usage;
        vk::MemoryPropertyFlags property_flags;

        BufferData(vk::raii::PhysicalDevice const& physical_device,
                   vk::raii::Device const&         device,
                   vk::DeviceSize                  size,
                   vk::BufferUsageFlags            usage,
                   vk::MemoryPropertyFlags         property_flags = vk::MemoryPropertyFlagBits::eHostVisible |
                                                            vk::MemoryPropertyFlagBits::eHostCoherent)
            : buffer(device, vk::BufferCreateInfo({}, size, usage))
#if defined(MEOW_DEBUG)
            , device_size(size)
            , usage(usage)
            , property_flags(property_flags)
#endif
        {
            device_memory = AllocateDeviceMemory(
                device, physical_device.getMemoryProperties(), buffer.getMemoryRequirements(), property_flags);
            buffer.bindMemory(*device_memory, 0);
        }

        BufferData(std::nullptr_t) {}

        template<typename DataType>
        void Upload(DataType const& data) const
        {
            assert((property_flags & vk::MemoryPropertyFlagBits::eHostCoherent) &&
                   (property_flags & vk::MemoryPropertyFlagBits::eHostVisible));
            assert(sizeof(DataType) <= device_size);

            void* data_ptr = device_memory.mapMemory(0, sizeof(DataType));
            memcpy(data_ptr, &data, sizeof(DataType));
            device_memory.unmapMemory();
        }

        template<typename DataType>
        void Upload(std::vector<DataType> const& data, size_t stride = 0) const
        {
            assert(property_flags & vk::MemoryPropertyFlagBits::eHostVisible);

            size_t element_size = stride ? stride : sizeof(DataType);
            assert(sizeof(DataType) <= element_size);

            CopyToDevice(device_memory, data.data(), data.size(), element_size);
        }

        template<typename DataType>
        void Upload(vk::raii::PhysicalDevice const& physical_device,
                    vk::raii::Device const&         device,
                    vk::raii::CommandPool const&    command_pool,
                    vk::raii::Queue const&          queue,
                    std::vector<DataType> const&    data,
                    size_t                          stride) const
        {
            assert(usage & vk::BufferUsageFlagBits::eTransferDst);
            assert(property_flags & vk::MemoryPropertyFlagBits::eDeviceLocal);

            size_t element_size = stride ? stride : sizeof(DataType);
            assert(sizeof(DataType) <= element_size);

            size_t dataSize = data.size() * element_size;
            assert(dataSize <= device_size);

            BufferData staging_buffer(physical_device, device, dataSize, vk::BufferUsageFlagBits::eTransferSrc);
            CopyToDevice(staging_buffer.device_memory, data.data(), data.size(), element_size);

            OneTimeSubmit(device, command_pool, queue, [&](vk::raii::CommandBuffer const& command_buffer) {
                command_buffer.copyBuffer(*staging_buffer.buffer, *this->buffer, vk::BufferCopy(0, 0, dataSize));
            });
        }
    };
} // namespace Meow
