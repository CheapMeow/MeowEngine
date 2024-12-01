#include "render_path.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    void swap(RenderPath& lhs, RenderPath& rhs)
    {
        using std::swap;

        swap(lhs.m_depth_format, rhs.m_depth_format);
        swap(lhs.m_depth_attachment, rhs.m_depth_attachment);

#ifdef MEOW_EDITOR
        swap(lhs.m_imgui_pass, rhs.m_imgui_pass);
        swap(lhs.m_offscreen_render_target, rhs.m_offscreen_render_target);
#endif
    }
} // namespace Meow
