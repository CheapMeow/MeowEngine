#include "vulkan_update_utils.h"

namespace Meow
{
    void UpdateDescriptorSets(
        vk::raii::Device const&        device,
        vk::raii::DescriptorSet const& descriptor_set,
        std::vector<
            std::tuple<vk::DescriptorType, vk::raii::Buffer const&, vk::DeviceSize, vk::raii::BufferView const*>> const&
                           buffer_data,
        TextureData const& texture_data,
        uint32_t           binding_offset)
    {
        std::vector<vk::DescriptorBufferInfo> buffer_infos;
        buffer_infos.reserve(buffer_data.size());

        std::vector<vk::WriteDescriptorSet> write_descriptor_sets;
        write_descriptor_sets.reserve(buffer_data.size() + 1);
        uint32_t dst_binding = binding_offset;
        for (auto const& bd : buffer_data)
        {
            buffer_infos.emplace_back(*std::get<1>(bd), 0, std::get<2>(bd));
            vk::BufferView buffer_view;
            if (std::get<3>(bd))
            {
                buffer_view = **std::get<3>(bd);
            }
            write_descriptor_sets.emplace_back(*descriptor_set,                         // dstSet
                                               dst_binding++,                           // dstBinding
                                               0,                                       // dstArrayElement
                                               1,                                       // descriptorCount
                                               std::get<0>(bd),                         // descriptorType
                                               nullptr,                                 // pImageInfo
                                               &buffer_infos.back(),                    // pBufferInfo
                                               std::get<3>(bd) ? &buffer_view : nullptr // pTexelBufferView
            );
        }

        vk::DescriptorImageInfo imageInfo(
            *texture_data.sampler, *texture_data.image_data.image_view, vk::ImageLayout::eShaderReadOnlyOptimal);
        write_descriptor_sets.emplace_back(*descriptor_set,                           // dstSet
                                           dst_binding,                               // dstBinding
                                           0,                                         // dstArrayElement
                                           vk::DescriptorType::eCombinedImageSampler, // descriptorType
                                           imageInfo,                                 // pImageInfo
                                           nullptr,                                   // pBufferInfo
                                           nullptr                                    // pTexelBufferView
        );

        device.updateDescriptorSets(write_descriptor_sets, nullptr);
    }

    void UpdateDescriptorSets(
        vk::raii::Device const&        device,
        vk::raii::DescriptorSet const& descriptor_set,
        std::vector<
            std::tuple<vk::DescriptorType, vk::raii::Buffer const&, vk::DeviceSize, vk::raii::BufferView const*>> const&
                                        buffer_data,
        std::vector<TextureData> const& texture_data,
        uint32_t                        binding_offset)
    {
        std::vector<vk::DescriptorBufferInfo> buffer_infos;
        buffer_infos.reserve(buffer_data.size());

        std::vector<vk::WriteDescriptorSet> write_descriptor_sets;
        write_descriptor_sets.reserve(buffer_data.size() + (texture_data.empty() ? 0 : 1));
        uint32_t dst_binding = binding_offset;
        for (auto const& bd : buffer_data)
        {
            buffer_infos.emplace_back(*std::get<1>(bd), 0, std::get<2>(bd));
            vk::BufferView buffer_view;
            if (std::get<3>(bd))
            {
                buffer_view = **std::get<3>(bd);
            }
            write_descriptor_sets.emplace_back(*descriptor_set,                         // dstSet
                                               dst_binding++,                           // dstBinding
                                               0,                                       // dstArrayElement
                                               1,                                       // descriptorCount
                                               std::get<0>(bd),                         // descriptorType
                                               nullptr,                                 // pImageInfo
                                               &buffer_infos.back(),                    // pBufferInfo
                                               std::get<3>(bd) ? &buffer_view : nullptr // pTexelBufferView
            );
        }

        std::vector<vk::DescriptorImageInfo> image_infos;
        if (!texture_data.empty())
        {
            image_infos.reserve(texture_data.size());
            for (auto const& thd : texture_data)
            {
                image_infos.emplace_back(
                    *thd.sampler, *thd.image_data.image_view, vk::ImageLayout::eShaderReadOnlyOptimal);
            }
            write_descriptor_sets.emplace_back(*descriptor_set,                            // dstSet
                                               dst_binding,                                // dstBinding
                                               0,                                          // dstArrayElement
                                               checked_cast<uint32_t>(image_infos.size()), // descriptorCount
                                               vk::DescriptorType::eCombinedImageSampler,  // descriptorType
                                               image_infos.data(),                         // pImageInfo
                                               nullptr,                                    // pBufferInfo
                                               nullptr                                     // pTexelBufferView
            );
        }

        device.updateDescriptorSets(write_descriptor_sets, nullptr);
    }
} // namespace Meow
