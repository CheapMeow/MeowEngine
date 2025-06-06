#include "model_mesh.h"

#include "pch.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    void ModelMesh::RefreshBuffer()
    {
        const vk::raii::PhysicalDevice& physical_device = g_runtime_context.render_system->GetPhysicalDevice();
        const vk::raii::Device&         logical_device  = g_runtime_context.render_system->GetLogicalDevice();
        const vk::raii::CommandPool&    onetime_submit_command_pool =
            g_runtime_context.render_system->GetOneTimeSubmitCommandPool();
        const vk::raii::Queue& graphics_queue = g_runtime_context.render_system->GetGraphicsQueue();

        if (!vertices.empty())
        {
            vertex_buffer_ptr = std::make_shared<VertexBuffer>(physical_device,
                                                               logical_device,
                                                               onetime_submit_command_pool,
                                                               graphics_queue,
                                                               vertices.size() * sizeof(float));
            vertex_buffer_ptr->Upload(
                physical_device, logical_device, onetime_submit_command_pool, graphics_queue, vertices, 0);
        }
        if (!indices.empty())
        {
            index_buffer_ptr = std::make_shared<IndexBuffer>(physical_device,
                                                             logical_device,
                                                             onetime_submit_command_pool,
                                                             graphics_queue,
                                                             indices.size() * sizeof(uint32_t));
            index_buffer_ptr->Upload(
                physical_device, logical_device, onetime_submit_command_pool, graphics_queue, indices, 0);
        }
    }

    void ModelMesh::BindOnly(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        if (vertex_buffer_ptr)
        {
            command_buffer.bindVertexBuffers(0, {*vertex_buffer_ptr->buffer}, {vertex_buffer_ptr->offset});
        }

        if (instance_buffer_ptr)
        {
            command_buffer.bindVertexBuffers(0, {*instance_buffer_ptr->buffer}, {instance_buffer_ptr->offset});
        }

        if (index_buffer_ptr)
        {
            command_buffer.bindIndexBuffer(*index_buffer_ptr->buffer, 0, index_buffer_ptr->index_type);
        }
    }

    void ModelMesh::DrawOnly(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        if (vertex_buffer_ptr && index_buffer_ptr)
        {
            command_buffer.drawIndexed(index_buffer_ptr->data_number, 1, 0, 0, 0);
        }
        else if (vertex_buffer_ptr)
        {
            command_buffer.draw(vertex_count, 1, 0, 0);
        }
    }

    void ModelMesh::BindDrawCmd(const vk::raii::CommandBuffer& command_buffer)
    {
        FUNCTION_TIMER();

        if (!vertex_buffer_ptr)
        {
            MEOW_ERROR("Doesn't have vertex buffer!");
            return;
        }

        BindOnly(command_buffer);
        DrawOnly(command_buffer);
    }
}; // namespace Meow