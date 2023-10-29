#pragma once

#include "function/ecs/component.h"
#include "function/renderer/structs/model.h"

#include <vector>

namespace Meow
{
    struct ModelComponent : Component
    {
        ModelComponent(const std::string&              file_name,
                       vk::raii::PhysicalDevice const& physical_device,
                       vk::raii::Device const&         device,
                       std::vector<VertexAttribute>    attributes)
            : model(file_name, physical_device, device, attributes)
        {}

        Model model;
    };
} // namespace Meow