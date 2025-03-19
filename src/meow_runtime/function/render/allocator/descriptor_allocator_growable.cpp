#include "descriptor_allocator_growable.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    DescriptorAllocatorGrowable::DescriptorAllocatorGrowable(const vk::raii::Device&             logical_device,
                                                             uint32_t                            initialSets,
                                                             std::vector<vk::DescriptorPoolSize> pool_sizes)
    {
        this->pool_sizes = pool_sizes;

        setsPerPool = initialSets * 1.5; // grow it next allocation

        vk::DescriptorPoolCreateInfo descriptor_pool_create_info(
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, initialSets, pool_sizes);
        readyPools.push_back(std::make_shared<vk::raii::DescriptorPool>(logical_device, descriptor_pool_create_info));
    }

    void DescriptorAllocatorGrowable::ClearPools()
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

    std::shared_ptr<vk::raii::DescriptorPool> DescriptorAllocatorGrowable::PopPool()
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

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
            newPool = std::make_shared<vk::raii::DescriptorPool>(logical_device, descriptor_pool_create_info);

            setsPerPool = setsPerPool * 1.5;
            if (setsPerPool > 4092)
            {
                setsPerPool = 4092;
            }
        }

        return newPool;
    }

    vk::raii::DescriptorSets
    DescriptorAllocatorGrowable::Allocate(std::vector<vk::DescriptorSetLayout> descriptor_set_layouts, void* pNext)
    {
        const vk::raii::Device& logical_device = g_runtime_context.render_system->GetLogicalDevice();

        // get or create a pool to allocate from
        std::shared_ptr<vk::raii::DescriptorPool> poolToUse = PopPool();

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(
            **poolToUse, descriptor_set_layouts.size(), descriptor_set_layouts.data(), pNext);

        vk::raii::DescriptorSets descriptor_sets = nullptr;

        try
        {
            descriptor_sets = vk::raii::DescriptorSets(logical_device, descriptor_set_allocate_info);
        }
        catch (const std::exception& e)
        {
            MEOW_INFO("{}\nAllocation failed. Try again", e.what());

            fullPools.push_back(poolToUse);

            poolToUse                                   = PopPool();
            descriptor_set_allocate_info.descriptorPool = **poolToUse;

            descriptor_sets = vk::raii::DescriptorSets(logical_device, descriptor_set_allocate_info);
        }

        readyPools.push_back(poolToUse);

        return descriptor_sets;
    }
} // namespace Meow
