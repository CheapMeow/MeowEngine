#include "render_pass.h"

#include "pch.h"

#include "function/global/runtime_context.h"

namespace Meow
{
    void swap(RenderPass& lhs, RenderPass& rhs)
    {
        using std::swap;

#ifdef MEOW_EDITOR
        swap(lhs.m_pass_name, rhs.m_pass_name);
        swap(lhs.m_query_pool, rhs.m_query_pool);
#endif
    }
} // namespace Meow