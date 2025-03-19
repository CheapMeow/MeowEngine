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

        DescriptorAllocatorGrowable(const vk::raii::Device&             logical_device,
                                    uint32_t                            initialSets,
                                    std::vector<vk::DescriptorPoolSize> pool_sizes);

        void ClearPools();

        vk::raii::DescriptorSets Allocate(std::vector<vk::DescriptorSetLayout> descriptor_set_layouts,
                                          void*                                pNext = nullptr);

    private:
        std::shared_ptr<vk::raii::DescriptorPool> PopPool();

        std::vector<vk::DescriptorPoolSize>                    pool_sizes;
        std::vector<std::shared_ptr<vk::raii::DescriptorPool>> fullPools;
        std::vector<std::shared_ptr<vk::raii::DescriptorPool>> readyPools;
        uint32_t                                               setsPerPool;
    };
} // namespace Meow
