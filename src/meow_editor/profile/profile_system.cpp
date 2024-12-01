#include "profile_system.h"

namespace Meow
{
    void ProfileSystem::Tick(float dt) {}

    void ProfileSystem::ClearProfile()
    {
        m_pipeline_stat_map.clear();
        m_render_stat_map.clear();
    }
} // namespace Meow