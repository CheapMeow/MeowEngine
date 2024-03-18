#pragma once

#include "index_buffer.h"
#include "vertex_buffer.h"

#include <memory>

namespace Meow
{
    struct ModelPrimitive
    {
        std::shared_ptr<IndexBuffer>  index_buffer_ptr    = nullptr;
        std::shared_ptr<VertexBuffer> vertex_buffer_ptr   = nullptr;
        std::shared_ptr<VertexBuffer> instance_buffer_ptr = nullptr;

        std::vector<float>    vertices;
        std::vector<uint16_t> indices;

        size_t vertex_count = 0;
        size_t triangle_num = 0;

        void DrawOnly(const vk::raii::CommandBuffer& cmd_buffer)
        {
            if (vertex_buffer_ptr && !index_buffer_ptr)
            {
                cmd_buffer.draw(vertex_count, 1, 0, 0);
            }
            else
            {
                cmd_buffer.drawIndexed(index_buffer_ptr->index_count, 1, 0, 0, 0);
            }
        }

        void BindOnly(const vk::raii::CommandBuffer& cmd_buffer)
        {
            if (vertex_buffer_ptr)
            {
                cmd_buffer.bindVertexBuffers(
                    0, {*vertex_buffer_ptr->buffer_data_ptr->buffer}, {vertex_buffer_ptr->offset});
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

        void BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer)
        {
            BindOnly(cmd_buffer);
            DrawOnly(cmd_buffer);
        }
    };
} // namespace Meow