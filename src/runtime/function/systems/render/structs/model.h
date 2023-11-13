#pragma once

#include "core/math/bounding_box.h"
#include "function/systems/render/structs/material.h"
#include "function/systems/render/structs/mesh.h"
#include "function/systems/render/structs/vertex_attribute.h"

#include <assimp/scene.h>
#include <glm/glm.hpp>

#include <filesystem>

namespace Meow
{
    struct Model
    {
        std::vector<Mesh> meshes;
        BoundingBox       bounding;

        Model(vk::raii::PhysicalDevice const& physical_device,
              vk::raii::Device const&         device,
              vk::raii::CommandPool const&    command_pool,
              vk::raii::Queue const&          queue,
              const std::string&              file_path,
              std::vector<VertexAttribute>    attributes,
              vk::IndexType                   index_type);

        BoundingBox GetBounding() { return bounding; }
    };
} // namespace Meow
