#pragma once

#include "function/render/model/vertex_attribute.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace Meow
{
    class GeometryFactory
    {
    public:
        void SetPlane();
        void SetCube();
        void SetSphere(uint32_t sector_count, uint32_t stack_count);
        void SetAttribute(VertexAttributeBit attribute_bit, const std::vector<float>& data);

        /**
         * @brief Get the vertices data for rendering.
         * If last attribute bits is different with input attribute bits, vertices data should be repopuldated.
         * Else output vertices buffer directly.
         *
         * @param attribute_bits Attribute bits. The order matters.
         * @return std::vector<float> vertices data for rendering.
         */
        std::vector<float>    GetVertices(const std::vector<Meow::VertexAttributeBit>& attribute_bits);
        std::vector<uint32_t> GetIndices() { return indices; }

        void clear();

    private:
        uint32_t                                                   vertices_number = 0;
        std::vector<float>                                         position;
        std::vector<uint32_t>                                      indices;
        std::unordered_map<VertexAttributeBit, std::vector<float>> attributes_map;

        /**
         * @brief Stores vertices output.
         * If no attributes changed, this buffer will not be populated.
         *
         */
        std::vector<float> vertices_buffer;

        /**
         * @brief Last vector of attribute bits used for output.
         * If last attribute bits is different with input attribute bits, vertices data should be repopuldated.
         * Else output vertices buffer directly.
         */
        std::vector<Meow::VertexAttributeBit> last_attribute_bits;
    };
} // namespace Meow
