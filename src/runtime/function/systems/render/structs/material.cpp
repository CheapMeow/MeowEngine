#include "material.h"

namespace Meow
{
    bool CompareMaterial(MaterialInfo dest, MaterialInfo source)
    {
        if (dest.diffuse_texture_index == source.diffuse_texture_index &&
            dest.normal_texture_index == source.normal_texture_index &&
            dest.specular_texture_index == source.specular_texture_index)
            return true;
        else
            return false;
    }
} // namespace Meow