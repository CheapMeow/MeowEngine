#pragma once

#include "core/math/bounding_box.h"
#include "function/render/buffer_data/index_buffer.h"
#include "function/render/buffer_data/vertex_buffer.h"

#include <memory>
#include <string>
#include <vector>

namespace Meow
{
    struct ModelNode;

    struct ModelMesh
    {
        std::shared_ptr<IndexBuffer>  index_buffer_ptr    = nullptr;
        std::shared_ptr<VertexBuffer> vertex_buffer_ptr   = nullptr;
        std::shared_ptr<VertexBuffer> instance_buffer_ptr = nullptr;

        std::vector<float>    vertices;
        std::vector<uint32_t> indices;

        size_t vertex_count   = 0;
        size_t triangle_count = 0;

        BoundingBox bounding;
        ModelNode*  link_node = nullptr;

        std::vector<size_t> bones;
        bool                isSkin = false;

        void RefreshBuffer();

        void BindOnly(const vk::raii::CommandBuffer& command_buffer);

        void DrawOnly(const vk::raii::CommandBuffer& command_buffer);

        void BindDrawCmd(const vk::raii::CommandBuffer& command_buffer);

        ~ModelMesh() { link_node = nullptr; }
    };

} // namespace Meow
