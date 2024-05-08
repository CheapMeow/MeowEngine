#include "render_pass.h"

namespace Meow
{
    void RenderPass::Start(vk::raii::CommandBuffer const& command_buffer,
                           Meow::SurfaceData const&       surface_data,
                           uint32_t                       current_image_index)
    {
        // debug
        if (enable_query)
            command_buffer.resetQueryPool(*query_pool, 0, 1);

        vk::RenderPassBeginInfo render_pass_begin_info(*render_pass,
                                                       *framebuffers[current_image_index],
                                                       vk::Rect2D(vk::Offset2D(0, 0), surface_data.extent),
                                                       clear_values);
        command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);
    }
} // namespace Meow