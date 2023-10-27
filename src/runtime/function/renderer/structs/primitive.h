#pragma once

#include "index_buffer.h"
#include "vertex_buffer.h"

namespace Meow
{
    // TODO: Support uint32_t indices
    struct Primitive
    {
        std::vector<float>    vertices;
        std::vector<uint16_t> indices;

        int32_t vertex_count = 0;
        int32_t index_count  = 0;

        VertexBuffer vertex_buffer;
        IndexBuffer  index_buffer;

        Primitive(std::vector<float>&&            _vertices,
                  std::vector<uint16_t>&&         _indices,
                  vk::raii::PhysicalDevice const& physical_device,
                  vk::raii::Device const&         device,
                  vk::MemoryPropertyFlags         property_flags = vk::MemoryPropertyFlagBits::eHostVisible |
                                                           vk::MemoryPropertyFlagBits::eHostCoherent,
                  std::vector<VertexAttribute> vertex_attributes = {},
                  vk::IndexType                index_type        = vk::IndexType::eUint16
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                  ,
                  std::string primitive_name = "Default Primitive Name"
#endif
                  )
            : vertices(std::move(_vertices))
            , indices(std::move(_indices))
            , vertex_count(vertices.size() / VertexAttributesToSize(vertex_attributes))
            , index_count(indices.size() / 3)
            , vertex_buffer(physical_device,
                            device,
                            vertices.size(),
                            property_flags,
                            vertices.data(),
                            vertex_count / sizeof(float),
                            vertex_attributes
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                            ,
                            primitive_name
#endif
                            )
            , index_buffer(physical_device,
                           device,
                           indices.size(),
                           property_flags,
                           indices.data(),
                           indices.size() / sizeof(uint16_t),
                           index_type
#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
                           ,
                           primitive_name
#endif
              )
        {}

        void BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer) const
        {
            vertex_buffer.Bind(cmd_buffer);
            index_buffer.BindDraw(cmd_buffer);
        }
    };
} // namespace Meow