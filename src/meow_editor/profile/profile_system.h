#pragma once

#include "meow_runtime/function/system.h"
#include "render/structs/builtin_render_stat.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Meow
{
    class ProfileSystem : public System
    {
    public:
        void Start() override {};

        void Tick(float dt) override;

        void ClearProfile();

        void UploadPipelineStat(const std::string& pass_name, const std::vector<uint32_t>& stat)
        {
            m_pipeline_stat_map[pass_name] = stat;
        }

        const std::unordered_map<std::string, std::vector<uint32_t>>& GetPipelineStat() { return m_pipeline_stat_map; }

        void UploadBuiltinRenderStat(const std::string& pass_name, BuiltinRenderStat stat)
        {
            m_render_stat_map[pass_name] = stat;
        }

        const std::unordered_map<std::string, BuiltinRenderStat>& GetBuiltinRenderStat() { return m_render_stat_map; }

    private:
        std::unordered_map<std::string, std::vector<uint32_t>> m_pipeline_stat_map;
        std::unordered_map<std::string, BuiltinRenderStat>     m_render_stat_map;
    };
} // namespace Meow
