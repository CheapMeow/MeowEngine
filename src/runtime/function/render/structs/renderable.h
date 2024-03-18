#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace Meow
{
    struct Renderable
    {
        virtual void BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer) const = 0;
    };

} // namespace Meow
