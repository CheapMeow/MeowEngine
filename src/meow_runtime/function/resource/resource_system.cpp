#include "resource_system.hpp"

namespace Meow
{
    template<>
    UUID ResourceSystem::Register(std::shared_ptr<Material> resource)
    {
        m_resources[resource->uuid()] = resource;

        std::cout << "12345" << std::endl;

        if (materials_id_per_shading_model.find(resource->GetShadingModelType()) ==
            materials_id_per_shading_model.end())
        {
            materials_id_per_shading_model[resource->GetShadingModelType()] = std::vector<UUID> {resource->uuid()};
        }
        else
        {
            materials_id_per_shading_model[resource->GetShadingModelType()].push_back(resource->uuid());
        }

        return resource->uuid();
    }

    std::vector<UUID>* ResourceSystem::GetMaterials(ShadingModelType shading_model_type)
    {
        if (materials_id_per_shading_model.find(shading_model_type) == materials_id_per_shading_model.end())
        {
            return nullptr;
        }

        return &materials_id_per_shading_model[shading_model_type];
    }
} // namespace Meow