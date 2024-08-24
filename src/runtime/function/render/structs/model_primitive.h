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

        void DrawOnly(const vk::raii::CommandBuffer& cmd_buffer);
        void BindOnly(const vk::raii::CommandBuffer& cmd_buffer);
        void BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer);
    };
} // namespace Meow