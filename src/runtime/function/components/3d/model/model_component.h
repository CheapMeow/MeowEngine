#pragma once

#include "function/components/component.h"
#include "function/systems/render/structs/model.h"

#include <vector>

namespace Meow
{
    struct ModelComponent : Component
    {
        Model model;

        ModelComponent(vk::raii::PhysicalDevice const& physical_device,
                       vk::raii::Device const&         device,
                       vk::raii::CommandPool const&    command_pool,
                       vk::raii::Queue const&          queue,
                       const std::string&              file_path,
                       std::vector<VertexAttribute>    attributes,
                       vk::IndexType                   index_type = vk::IndexType::eUint16)
            : model(physical_device, device, command_pool, queue, file_path, attributes, index_type)
        {}
    };
} // namespace Meow