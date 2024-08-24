#include "model_mesh.h"

#include "pch.h"

namespace Meow
{
    void ModelMesh::BindOnly(const vk::raii::CommandBuffer& cmd_buffer)
    {
        FUNCTION_TIMER();

        if (vertex_buffer_ptr)
        {
            cmd_buffer.bindVertexBuffers(0, {*vertex_buffer_ptr->buffer_data_ptr->buffer}, {vertex_buffer_ptr->offset});
        }

        if (instance_buffer_ptr)
        {
            cmd_buffer.bindVertexBuffers(
                0, {*instance_buffer_ptr->buffer_data_ptr->buffer}, {instance_buffer_ptr->offset});
        }

        if (index_buffer_ptr)
        {
            cmd_buffer.bindIndexBuffer(*index_buffer_ptr->buffer_data_ptr->buffer, 0, index_buffer_ptr->index_type);
        }
    }

    void ModelMesh::DrawOnly(const vk::raii::CommandBuffer& cmd_buffer)
    {
        FUNCTION_TIMER();

        if (vertex_buffer_ptr && !index_buffer_ptr)
        {
            cmd_buffer.draw(vertex_count, 1, 0, 0);
        }
        else
        {
            cmd_buffer.drawIndexed(index_buffer_ptr->index_count, 1, 0, 0, 0);
        }
    }

    void ModelMesh::BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer)
    {
        FUNCTION_TIMER();

        BindOnly(cmd_buffer);
        DrawOnly(cmd_buffer);
    }

    void ModelMesh::Merge(ModelMesh& rhs)
    {
        if (link_node != rhs.link_node)
            return;
    }
}; // namespace Meow