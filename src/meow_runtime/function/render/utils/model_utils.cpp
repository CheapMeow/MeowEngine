#include "model_utils.h"

#include <cmath>
#include <numbers>

namespace Meow
{
    std::tuple<std::vector<float>, std::vector<uint32_t>>
    GenerateSphereVerticesAndIndices(uint32_t                        x_segments,
                                     uint32_t                        y_segments,
                                     std::vector<VertexAttributeBit> attributes)
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<uint32_t>  indices;

        for (std::size_t x = 0; x <= x_segments; ++x)
        {
            for (std::size_t y = 0; y <= y_segments; ++y)
            {
                float u     = (float)x / (float)x_segments;
                float v     = (float)y / (float)y_segments;
                float x_pos = std::cos(u * 2.0f * std::numbers::pi) * std::sin(v * std::numbers::pi);
                float y_pos = std::cos(v * std::numbers::pi);
                float z_pos = std::sin(u * 2.0f * std::numbers::pi) * std::sin(v * std::numbers::pi);

                positions.push_back(glm::vec3(x_pos, y_pos, z_pos));
                uv.push_back(glm::vec2(u, v));
                normals.push_back(glm::vec3(x_pos, y_pos, z_pos));
            }
        }

        bool odd_row = false;
        for (std::size_t y = 0; y < y_segments; ++y)
        {
            if (!odd_row) // even rows: y == 0, y == 2; and so on
            {
                for (std::size_t x = 0; x <= x_segments; ++x)
                {
                    indices.push_back(y * (x_segments + 1) + x);
                    indices.push_back((y + 1) * (x_segments + 1) + x);
                }
            }
            else
            {
                for (int x = x_segments; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (x_segments + 1) + x);
                    indices.push_back(y * (x_segments + 1) + x);
                }
            }
            odd_row = !odd_row;
        }

        std::vector<float> data;
        for (std::size_t i = 0; i < positions.size(); ++i)
        {
            for (std::size_t j = 0; j < attributes.size(); ++j)
            {
                if (attributes[j] == VertexAttributeBit::Position)
                {
                    data.push_back(positions[i].x);
                    data.push_back(positions[i].y);
                    data.push_back(positions[i].z);
                }
                if (attributes[j] == VertexAttributeBit::UV0 || attributes[j] == VertexAttributeBit::UV1)
                {
                    data.push_back(uv[i].x);
                    data.push_back(uv[i].y);
                }
                if (attributes[j] == VertexAttributeBit::Normal)
                {
                    data.push_back(normals[i].x);
                    data.push_back(normals[i].y);
                    data.push_back(normals[i].z);
                }
            }
        }

        return {data, indices};
    }
} // namespace Meow