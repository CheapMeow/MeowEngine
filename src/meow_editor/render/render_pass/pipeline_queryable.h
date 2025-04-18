#pragma once

#include "core/base/non_copyable.h"

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

namespace Meow
{
    class PipelineQueryable : public NonCopyable
    {
    public:
        void CreateQueryPool(const vk::raii::Device& logical_device, uint32_t query_count);
        void BeginQuery(const vk::raii::CommandBuffer& command_buffer);
        void EndQuery(const vk::raii::CommandBuffer& command_buffer);
        void ResetQueryPool(const vk::raii::CommandBuffer& command_buffer);
        std::pair<vk::Result, std::vector<uint32_t>> GetQueryResults(uint32_t query_index);

        friend void swap(PipelineQueryable& lhs, PipelineQueryable& rhs);

    protected:
        vk::raii::QueryPool query_pool = nullptr;

        bool     m_query_enabled = true;
        bool     m_query_started = false;
        uint32_t m_query_count_max;
        uint32_t m_query_count_accumulated = 0;
    };
} // namespace Meow
