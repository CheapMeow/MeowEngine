#pragma once

#include "core/math/bounding_box.h"
#include "image_data.h"
#include "index_buffer.h"
#include "vertex_buffer.h"

#include <memory>
#include <string>
#include <vector>

namespace Meow
{
    struct ModelNode;

    struct TextureInfo
    {
        std::string diffuse;
        std::string normal;
        std::string specular;

        std::shared_ptr<ImageData> diffuse_texture;
        std::shared_ptr<ImageData> normal_texture;
        std::shared_ptr<ImageData> specular_texture;
    };

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

        TextureInfo texture_info;

        void RefreshBuffer();
        
        void BindOnly(const vk::raii::CommandBuffer& cmd_buffer);

        void DrawOnly(const vk::raii::CommandBuffer& cmd_buffer);

        void BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer);

        ~ModelMesh() { link_node = nullptr; }
    };

} // namespace Meow
