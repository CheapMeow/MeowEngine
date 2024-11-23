#pragma once

#include "core/uuid/uuid.h"
#include "function/render/render_resources/image_data.h"
#include "function/render/render_resources/model.hpp"
#include "function/render/structs/shader.h"
#include "function/system.h"

#include <tuple>
#include <unordered_map>
#include <vector>

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
        ResourceSystem();
        ~ResourceSystem();

        void Start() override;

        void Tick(float dt) override;

        std::tuple<bool, UUID> LoadTexture(const std::string& file_path);

        std::shared_ptr<ImageData> GetTexture(const UUID& uuid);

        // bool LoadMaterial(const std::string& file_path, UUID& uuid);

        // std::shared_ptr<Material> GetMaterial(const UUID& uuid);

        template<typename VerticesType, typename IndicesType>
        std::tuple<bool, UUID>
        LoadModel(VerticesType&& vertices, IndicesType&& indices, std::vector<VertexAttributeBit> attributes)
        {
            FUNCTION_TIMER();

            auto model_ptr = std::make_shared<Model>(
                std::forward<VerticesType>(vertices), std::forward<IndicesType>(indices), attributes);

            if (model_ptr)
            {
                m_models_id2data[model_ptr->uuid] = model_ptr;
                return {true, model_ptr->uuid};
            }
            else
            {
                return {false, UUID(0)};
            }
        }

        std::tuple<bool, UUID> LoadModel(const std::string& file_path, std::vector<VertexAttributeBit> attributes);

        std::shared_ptr<Model> GetModel(const UUID& uuid);

    private:
        std::unordered_map<std::string, UUID>                m_textures_path2id;
        std::unordered_map<UUID, std::shared_ptr<ImageData>> m_textures_id2data;

        // std::unordered_map<UUID, std::shared_ptr<Material>> m_materials;

        std::unordered_map<std::string, UUID>            m_models_path2id;
        std::unordered_map<UUID, std::shared_ptr<Model>> m_models_id2data;
    };
} // namespace Meow
