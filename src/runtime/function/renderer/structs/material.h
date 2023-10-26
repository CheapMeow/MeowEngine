#pragma once

#include <cstdint>

namespace Meow
{
    /**
     * @brief Indices to texture.
     *
     * Max of uint32_t means no texture.
     *
     * @todo TODO: Support more complex situation, such as multi texture.
     */
    struct MaterialInfo
    {
        uint32_t diffuse_texture_index;
        uint32_t normal_texture_index;
        uint32_t specular_texture_index;

        MaterialInfo(uint32_t _diffuse_texture_index, uint32_t _normal_texture_index, uint32_t _specular_texture_index)
            : diffuse_texture_index(_diffuse_texture_index)
            , normal_texture_index(_normal_texture_index)
            , specular_texture_index(_specular_texture_index)
        {}
    };

    bool CompareMaterial(MaterialInfo dest, MaterialInfo source);
} // namespace Meow
