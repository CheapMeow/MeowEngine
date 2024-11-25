#include "model_utils.h"

#include <cmath>
#include <numbers>

namespace Meow
{
    std::vector<float> GenerateCubeVertices()
    {
        // clang-format off
        return {
            // back face
            -1.0f, -1.0f, -1.0f, // bottom-left
             1.0f,  1.0f, -1.0f, // top-right
             1.0f, -1.0f, -1.0f, // bottom-right         
             1.0f,  1.0f, -1.0f, // top-right
            -1.0f, -1.0f, -1.0f, // bottom-left
            -1.0f,  1.0f, -1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f, // bottom-left
             1.0f, -1.0f,  1.0f, // bottom-right
             1.0f,  1.0f,  1.0f, // top-right
             1.0f,  1.0f,  1.0f, // top-right
            -1.0f,  1.0f,  1.0f, // top-left
            -1.0f, -1.0f,  1.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, // top-right
            -1.0f,  1.0f, -1.0f, // top-left
            -1.0f, -1.0f, -1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f, // top-left
             1.0f, -1.0f, -1.0f, // bottom-right
             1.0f,  1.0f, -1.0f, // top-right         
             1.0f, -1.0f, -1.0f, // bottom-right
             1.0f,  1.0f,  1.0f, // top-left
             1.0f, -1.0f,  1.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f, // top-right
             1.0f, -1.0f, -1.0f, // top-left
             1.0f, -1.0f,  1.0f, // bottom-left
             1.0f, -1.0f,  1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, // bottom-right
            -1.0f, -1.0f, -1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f, // top-left
             1.0f,  1.0f , 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f, // top-right     
             1.0f,  1.0f,  1.0f, // bottom-right
            -1.0f,  1.0f, -1.0f, // top-left
            -1.0f,  1.0f,  1.0f  // bottom-left        
        };
        // clang-format on
    }

    std::tuple<std::vector<float>, std::vector<uint32_t>>
    GenerateSphereVerticesAndIndices(uint32_t                        sector_count,
                                     uint32_t                        stack_count,
                                     float                           radius,
                                     std::vector<VertexAttributeBit> attributes)
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;

        float x, y, z, xy;                           // vertex position
        float nx, ny, nz, lengthInv = 1.0f / radius; // vertex normal
        float s, t;                                  // vertex texCoord

        float sectorStep = 2 * std::numbers::pi / sector_count;
        float stackStep  = std::numbers::pi / stack_count;
        float sectorAngle, stackAngle;

        for (int i = 0; i <= stack_count; ++i)
        {
            stackAngle = std::numbers::pi / 2 - i * stackStep; // starting from pi/2 to -pi/2
            xy         = radius * cosf(stackAngle);            // r * cos(u)
            z          = radius * sinf(stackAngle);            // r * sin(u)

            // add (sector_count+1) positions per stack
            // first and last positions have same position and normal, but different tex coords
            for (int j = 0; j <= sector_count; ++j)
            {
                sectorAngle = j * sectorStep; // starting from 0 to 2pi

                // vertex position (x, y, z)
                x = xy * cosf(sectorAngle); // r * cos(u) * cos(v)
                y = xy * sinf(sectorAngle); // r * cos(u) * sin(v)
                positions.push_back({x, y, z});

                // normalized vertex normal (nx, ny, nz)
                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;
                normals.push_back({nx, ny, nz});

                // vertex tex coord (s, t) range between [0, 1]
                s = (float)j / sector_count;
                t = (float)i / stack_count;
                uv.push_back({s, t});
            }
        }

        std::vector<uint32_t> indices;
        int                   k1, k2;
        for (int i = 0; i < stack_count; ++i)
        {
            k1 = i * (sector_count + 1); // beginning of current stack
            k2 = k1 + sector_count + 1;  // beginning of next stack

            for (int j = 0; j < sector_count; ++j, ++k1, ++k2)
            {
                // 2 triangles per sector excluding first and last stacks
                // k1 => k2 => k1+1
                if (i != 0)
                {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                // k1+1 => k2 => k2+1
                if (i != (stack_count - 1))
                {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
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