#include "descriptor_allocator_growable.h"

namespace Meow
{
    void DescriptorAllocatorGrowable::ClearPools(vk::raii::Device const& device)
    {
        for (auto p : readyPools)
        {
            (*p).reset(vk::DescriptorPoolResetFlags());
        }
        for (auto p : fullPools)
        {
            (*p).reset(vk::DescriptorPoolResetFlags());
            readyPools.push_back(p);
        }
        fullPools.clear();
    }

    std::shared_ptr<vk::raii::DescriptorPool> DescriptorAllocatorGrowable::PopPool(vk::raii::Device const& device)
    {
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, setsPerPool, pool_sizes);

        std::shared_ptr<vk::raii::DescriptorPool> newPool;
        if (readyPools.size() != 0)
        {
            newPool = readyPools.back();
            readyPools.pop_back();
        }
        else
        {
            // need to create a new pool
            newPool = std::make_shared<vk::raii::DescriptorPool>(device, descriptor_pool_create_info);

            setsPerPool = setsPerPool * 1.5;
            if (setsPerPool > 4092)
            {
                setsPerPool = 4092;
            }
        }

        return newPool;
    }

    vk::raii::DescriptorSets
    DescriptorAllocatorGrowable::Allocate(vk::raii::Device const&              device,
                                          std::vector<vk::DescriptorSetLayout> descriptor_set_layouts,
                                          void*                                pNext)
    {
        // get or create a pool to allocate from
        std::shared_ptr<vk::raii::DescriptorPool> poolToUse = PopPool(device);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(
            **poolToUse, descriptor_set_layouts.size(), descriptor_set_layouts.data(), pNext);

        vk::raii::DescriptorSets descriptor_sets = nullptr;

        try
        {
            descriptor_sets = vk::raii::DescriptorSets(device, descriptor_set_allocate_info);
        }
        catch (const std::exception& e)
        {
            RUNTIME_INFO("{}\nAllocation failed. Try again", e.what());

            fullPools.push_back(poolToUse);

            poolToUse                                   = PopPool(device);
            descriptor_set_allocate_info.descriptorPool = **poolToUse;

            descriptor_sets = vk::raii::DescriptorSets(device, descriptor_set_allocate_info);
        }

        readyPools.push_back(poolToUse);

        return descriptor_sets;
    }
} // namespace Meow
