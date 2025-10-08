#include "pipeline_queryable.h"

#include "pch.h"

#include "function/global/runtime_context.h"
#include "function/render/utils/vulkan_initialization_utils.hpp"

namespace Meow
{
    void PipelineQueryable::CreateQueryPool(const vk::raii::Device& logical_device, uint32_t query_count)
    {
        VkQueryPoolCreateInfo query_pool_create_info = {.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
                                                        .queryType          = VK_QUERY_TYPE_PIPELINE_STATISTICS,
                                                        .queryCount         = query_count,
                                                        .pipelineStatistics = (1 << 11) - 1};

        query_pool = logical_device.createQueryPool(query_pool_create_info, nullptr);

        m_query_count_max = query_count;

        const vk::raii::CommandPool& onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        // At first use, all queries should be reset
        OneTimeSubmit(logical_device,
                      onetime_submit_command_pool,
                      graphics_queue,
                      [this](vk::raii::CommandBuffer& command_buffer) { ResetQueryPool(command_buffer); });
    }

    void PipelineQueryable::BeginQuery(const vk::raii::CommandBuffer& command_buffer)
    {
        if (!m_query_enabled)
            return;

        if (m_query_count_accumulated >= m_query_count_max)
        {
            MEOW_ERROR("Query count exceeded the maximum limit.");
            return;
        }

        m_query_started = true;

        command_buffer.beginQuery(*query_pool, m_query_count_accumulated, {});
    }

    void PipelineQueryable::EndQuery(const vk::raii::CommandBuffer& command_buffer)
    {
        if (!m_query_started)
            return;

        m_query_started = false;

        command_buffer.endQuery(*query_pool, m_query_count_accumulated);
        m_query_count_accumulated++;
    }

    void PipelineQueryable::ResetQueryPool(const vk::raii::CommandBuffer& command_buffer)
    {
        command_buffer.resetQueryPool(*query_pool, 0, m_query_count_max);
        m_query_count_accumulated = 0;
    }

    std::pair<vk::Result, std::vector<uint32_t>> PipelineQueryable::GetQueryResults(uint32_t query_index)
    {
        if (query_index >= m_query_count_max)
        {
            MEOW_ERROR("Query index out of range.");
            return {vk::Result::eErrorOutOfDateKHR, {}};
        }

        return query_pool.getResults<uint32_t>(query_index, 1, sizeof(uint32_t) * 11, sizeof(uint32_t) * 11, {});
    }

    void swap(PipelineQueryable& lhs, PipelineQueryable& rhs)
    {
        using std::swap;

        swap(lhs.query_pool, rhs.query_pool);
        swap(lhs.m_query_enabled, rhs.m_query_enabled);
        swap(lhs.m_query_started, rhs.m_query_started);
        swap(lhs.m_query_count_max, rhs.m_query_count_max);
        swap(lhs.m_query_count_accumulated, rhs.m_query_count_accumulated);
    }
} // namespace Meow