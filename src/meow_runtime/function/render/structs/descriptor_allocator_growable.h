#pragma once

#include "pch.h"

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    struct DescriptorAllocatorGrowable
    {
    public:
        DescriptorAllocatorGrowable() {}
        DescriptorAllocatorGrowable(std::nullptr_t) {}

        DescriptorAllocatorGrowable(vk::raii::Device const&             device,
                                    uint32_t                            initialSets,
                                    std::vector<vk::DescriptorPoolSize> pool_sizes)
        {
            this->pool_sizes = pool_sizes;

            setsPerPool = initialSets * 1.5; // grow it next allocation

            vk::DescriptorPoolCreateInfo descriptor_pool_create_info = {
                .flags         = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                .maxSets       = initialSets,
                .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
                .pPoolSizes    = pool_sizes.data(),
            };
            readyPools.push_back(std::make_shared<vk::raii::DescriptorPool>(device, descriptor_pool_create_info));
        }

        void ClearPools(vk::raii::Device const& device);

        vk::raii::DescriptorSets Allocate(vk::raii::Device const&              device,
                                          std::vector<vk::DescriptorSetLayout> descriptor_set_layouts,
                                          void*                                pNext = nullptr);

    private:
        std::shared_ptr<vk::raii::DescriptorPool> PopPool(vk::raii::Device const& device);

        std::vector<vk::DescriptorPoolSize>                    pool_sizes;
        std::vector<std::shared_ptr<vk::raii::DescriptorPool>> fullPools;
        std::vector<std::shared_ptr<vk::raii::DescriptorPool>> readyPools;
        uint32_t                                               setsPerPool;
    };
} // namespace Meow
