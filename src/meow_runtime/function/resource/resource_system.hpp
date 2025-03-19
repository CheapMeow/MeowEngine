#pragma once

#include "core/uuid/uuid.h"
#include "function/render/buffer_data/image_data.h"
#include "function/render/render_resources/model.hpp"
#include "function/render/render_resources/shader.h"
#include "function/system.h"
#include "resource_base.h"

#include <unordered_map>

namespace Meow
{
    /**
     * @brief Beside loading resource from disk to memory,
     * Resource System can also handle sharing, solving dependencies of resources, reloading.
     *
     * 1. Sharing resources
     *
     * If game object A has load resource Res1, and then new a game object B which also need Res1,
     * Resource System can directly provide Res1 for B, but not load a duplicate of Res1.
     *
     * 2. Solving Dependencies
     *
     * If loading a game object need loading resource Res1, and Res1 depends on Res2, Res3,
     * Resource System will load Res2, Res3 firstly, and then load Res1.
     *
     * 3. Reloading from disk
     *
     * When engine is running, it will auto detect all resource files and reload the updated files.
     */
    class ResourceSystem final : public System
    {
    public:
        ResourceSystem()  = default;
        ~ResourceSystem() = default;

        void Start() override {}

        void Tick(float dt) override {}

        template<typename ResourceType>
        UUID Register(std::shared_ptr<ResourceType> res_ptr)
        {
            m_resources[res_ptr->uuid] = res_ptr;
            return res_ptr->uuid;
        }

        template<typename ResourceType>
        std::shared_ptr<ResourceType> GetResource(UUID uuid)
        {
            auto it = m_resources.find(uuid);
            if (it == m_resources.end())
                return nullptr;

            return std::dynamic_pointer_cast<ResourceType>(it->second);
        }

    private:
        std::unordered_map<UUID, std::shared_ptr<ResourceBase>> m_resources;
    };

} // namespace Meow
