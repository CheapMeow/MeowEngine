#pragma once

#include "core/math/bounding_box.h"
#include "function/systems/render/structs/material.h"
#include "function/systems/render/structs/mesh.h"
#include "function/systems/render/structs/vertex_attribute.h"

#include <assimp/scene.h>
#include <glm/glm.hpp>

#include <filesystem>
#include <unordered_map>

namespace Meow
{
    struct Model
    {
        std::vector<Mesh> meshes;
        BoundingBox       bounding;

        std::vector<VertexAttribute> attributes;
        vk::IndexType                index_type = vk::IndexType::eUint16;

        Model(const std::string&              file_path,
              vk::raii::PhysicalDevice const& physical_device,
              vk::raii::Device const&         device,
              std::vector<VertexAttribute>    _attributes)
            : attributes(_attributes)
        {
            LoadFromFile(physical_device, device, file_path);
        }

        BoundingBox GetBounding() { return bounding; }

    private:
        void LoadFromFile(vk::raii::PhysicalDevice const& physical_device,
                          vk::raii::Device const&         device,
                          const std::string&              file_name);

        void LoadMesh(vk::raii::PhysicalDevice const& physical_device,
                      vk::raii::Device const&         device,
                      const aiMesh*                   aiMesh,
                      Mesh&                           mesh);
    };
} // namespace Meow
