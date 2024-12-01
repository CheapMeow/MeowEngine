#pragma once

#include "function/render/structs/material_stat.h"
#include "function/system.h"


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

        void UploadMaterialStat(const std::string& pass_name, MaterialStat stat)
        {
            m_render_stat_map[pass_name] = stat;
        }

        const std::unordered_map<std::string, MaterialStat>& GetMaterialStat() { return m_render_stat_map; }

    private:
        std::unordered_map<std::string, std::vector<uint32_t>> m_pipeline_stat_map;
        std::unordered_map<std::string, MaterialStat>          m_render_stat_map;
    };
} // namespace Meow
