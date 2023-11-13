#pragma once

#include "core/math/bounding_box.h"
#include "function/systems/render/structs/vertex_attribute.h"
#include "primitive.h"
#include "renderable.h"

#include <assimp/scene.h>

namespace Meow
{
    struct Mesh : Renderable
    {
        std::vector<std::shared_ptr<Primitive>> primitives;
        std::string                             material_name = "Default Material";
        BoundingBox                             bounding;

        int32_t vertex_count = 0;
        int32_t index_count  = 0;

        Mesh(vk::raii::PhysicalDevice const& physical_device,
             vk::raii::Device const&         device,
             vk::raii::CommandPool const&    command_pool,
             vk::raii::Queue const&          queue,
             const aiMesh*                   aiMesh,
             std::vector<VertexAttribute>    attributes,
             vk::IndexType                   index_type);

        void BindDrawCmd(vk::raii::CommandBuffer const& cmd_buffer) const override;
    };

} // namespace Meow