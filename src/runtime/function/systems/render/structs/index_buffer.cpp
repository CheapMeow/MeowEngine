#include "index_buffer.h"

namespace Meow
{
    void IndexBuffer::BindDraw(const vk::raii::CommandBuffer& cmd_buffer) const
    {
        cmd_buffer.bindIndexBuffer(*buffer_data.buffer, 0, type);
        cmd_buffer.drawIndexed(count, 1, 0, 0, 0);
    }
} // namespace Meow