#pragma once

#include "function/system.h"
#include "level.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Meow
{
    class LevelSystem : public System
    {
    public:
        void Start() override;

        void Tick(float dt) override;

        std::weak_ptr<Level> GetCurrentActiveLevel() const { return m_current_active_level; }

    private:
        std::unordered_map<std::string, std::shared_ptr<Level>> m_levels;
        std::weak_ptr<Level>                                    m_current_active_level;
    };
} // namespace Meow
