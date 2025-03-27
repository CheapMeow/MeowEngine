#include "pch.h"

#include "geometry_factory.h"

#include <glm/glm.hpp>

#include <cmath>
#include <numbers>
#include <unordered_set>

namespace Meow
{
    void GeometryFactory::SetPlane()
    {
        clear();

        // clang-format off
        position = {
            -1.0f,  0.0f, -1.0f, // top-left
            1.0f,  0.0f,  1.0f, // bottom-right
            1.0f,  0.0f, -1.0f, // top-right     
            1.0f,  0.0f,  1.0f, // bottom-right
            -1.0f,  0.0f, -1.0f, // top-left
            -1.0f,  0.0f,  1.0f  // bottom-left  
        };
        // clang-format on
        indices = {};

        // clang-format off
        std::vector<float> uv = {
            0.0f, 1.0f, // top-left
            1.0f, 0.0f, // bottom-right
            1.0f, 1.0f, // top-right
            1.0f, 0.0f, // bottom-right
            0.0f, 1.0f, // top-left
            0.0f, 0.0f  // bottom-left
        };
        // clang-format on

        // clang-format off
        std::vector<float> normal = {
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f
        };
        // clang-format on

        vertices_number = position.size() / 3;

        SetAttribute(VertexAttributeBit::Position, position);
        SetAttribute(VertexAttributeBit::UV0, uv);
        SetAttribute(VertexAttributeBit::Normal, normal);
    }

    void GeometryFactory::SetCube()
    {
        clear();

        // Normals are specified per-vertex, and since the normals for the three faces that share each vertex are
        // orthogonal, you'll get some really wonky looking results by specifying a cube with just 8 vertices and
        // averaging the three face normals to get the vertex normal. It'll be shaded as a sphere, but shaped like a
        // cube.
        //
        // You'll instead need to specify 24 vertices, so each face of the cube is drawn without sharing vertices with
        // any other.

        // clang-format off
        position = {
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
            1.0f,  1.0f,  1.0f, // bottom-right
            1.0f,  1.0f, -1.0f, // top-right     
            1.0f,  1.0f,  1.0f, // bottom-right
            -1.0f,  1.0f, -1.0f, // top-left
            -1.0f,  1.0f,  1.0f  // bottom-left        
        };
        // clang-format on
        indices = {};

        // clang-format off
        std::vector<float> uv = {
            // back face
            0.0f, 0.0f, // bottom-left
            1.0f, 1.0f, // top-right
            1.0f, 0.0f, // bottom-right
            1.0f, 1.0f, // top-right
            0.0f, 0.0f, // bottom-left
            0.0f, 1.0f, // top-left
            // front face
            0.0f, 0.0f, // bottom-left
            1.0f, 0.0f, // bottom-right
            1.0f, 1.0f, // top-right
            1.0f, 1.0f, // top-right
            0.0f, 1.0f, // top-left
            0.0f, 0.0f, // bottom-left
            // left face
            1.0f, 0.0f, // top-right
            0.0f, 0.0f, // top-left
            0.0f, 1.0f, // bottom-left
            0.0f, 1.0f, // bottom-left
            1.0f, 1.0f, // bottom-right
            1.0f, 0.0f, // top-right
            // right face
            0.0f, 0.0f, // top-left
            1.0f, 1.0f, // bottom-right
            1.0f, 0.0f, // top-right
            1.0f, 1.0f, // bottom-right
            0.0f, 0.0f, // top-left
            0.0f, 1.0f, // bottom-left
            // bottom face
            1.0f, 1.0f, // top-right
            0.0f, 1.0f, // top-left
            0.0f, 0.0f, // bottom-left
            0.0f, 0.0f, // bottom-left
            1.0f, 0.0f, // bottom-right
            1.0f, 1.0f, // top-right
            // top face
            0.0f, 1.0f, // top-left
            1.0f, 0.0f, // bottom-right
            1.0f, 1.0f, // top-right
            1.0f, 0.0f, // bottom-right
            0.0f, 1.0f, // top-left
            0.0f, 0.0f  // bottom-left
        };
        // clang-format on

        // Normal vectors for each vertex (same for all vertices in a face)
        // clang-format off
        std::vector<float> normal = {
            // back face (facing negative Z)
            0.0f,  0.0f, -1.0f,
            0.0f,  0.0f, -1.0f,
            0.0f,  0.0f, -1.0f,
            0.0f,  0.0f, -1.0f,
            0.0f,  0.0f, -1.0f,
            0.0f,  0.0f, -1.0f,
            // front face (facing positive Z)
            0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  1.0f,
            0.0f,  0.0f,  1.0f,
            // left face (facing negative X)
            -1.0f,  0.0f,  0.0f,
            -1.0f,  0.0f,  0.0f,
            -1.0f,  0.0f,  0.0f,
            -1.0f,  0.0f,  0.0f,
            -1.0f,  0.0f,  0.0f,
            -1.0f,  0.0f,  0.0f,
            // right face (facing positive X)
            1.0f,  0.0f,  0.0f,
            1.0f,  0.0f,  0.0f,
            1.0f,  0.0f,  0.0f,
            1.0f,  0.0f,  0.0f,
            1.0f,  0.0f,  0.0f,
            1.0f,  0.0f,  0.0f,
            // bottom face (facing negative Y)
            0.0f, -1.0f,  0.0f,
            0.0f, -1.0f,  0.0f,
            0.0f, -1.0f,  0.0f,
            0.0f, -1.0f,  0.0f,
            0.0f, -1.0f,  0.0f,
            0.0f, -1.0f,  0.0f,
            // top face (facing positive Y)
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f,
            0.0f,  1.0f,  0.0f
        };
        // clang-format on

        vertices_number = position.size() / 3;

        SetAttribute(VertexAttributeBit::Position, position);
        SetAttribute(VertexAttributeBit::UV0, uv);
        SetAttribute(VertexAttributeBit::Normal, normal);
    }

    void GeometryFactory::SetSphere(uint32_t sector_count, uint32_t stack_count)
    {
        clear();

        std::vector<float> uv;
        std::vector<float> normal;

        float x, y, z, xy;                  // vertex position
        float nx, ny, nz, lengthInv = 1.0f; // vertex normal
        float s, t;                         // vertex texCoord

        float sectorStep = 2 * std::numbers::pi / sector_count;
        float stackStep  = std::numbers::pi / stack_count;
        float sectorAngle, stackAngle;

        for (int i = 0; i <= stack_count; ++i)
        {
            stackAngle = std::numbers::pi / 2 - i * stackStep; // starting from pi/2 to -pi/2
            xy         = std::cos(stackAngle);                 // r * cos(u)
            z          = std::sin(stackAngle);                 // r * sin(u)

            // add (sector_count+1) position per stack
            // first and last position have same position and normal, but different tex coords
            for (int j = 0; j <= sector_count; ++j)
            {
                sectorAngle = j * sectorStep; // starting from 0 to 2pi

                // vertex position (x, y, z)
                x = xy * std::cos(sectorAngle); // r * cos(u) * cos(v)
                y = xy * std::sin(sectorAngle); // r * cos(u) * sin(v)
                position.push_back(x);
                position.push_back(y);
                position.push_back(z);

                // vertex tex coord (s, t) range between [0, 1]
                s = (float)j / sector_count;
                t = (float)i / stack_count;
                uv.push_back(s);
                uv.push_back(t);

                // normalized vertex normal (nx, ny, nz)
                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;
                normal.push_back(nx);
                normal.push_back(ny);
                normal.push_back(nz);
            }
        }

        int k1, k2;
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

        vertices_number = position.size() / 3;

        SetAttribute(VertexAttributeBit::Position, position);
        SetAttribute(VertexAttributeBit::UV0, uv);
        SetAttribute(VertexAttributeBit::Normal, normal);
    }

    void GeometryFactory::SetAttribute(VertexAttributeBit attribute_bit, const std::vector<float>& data)
    {
        if (vertices_number == 0)
        {
            MEOW_ERROR("Vertices number in factory is zero!");
            return;
        }

        uint32_t component_number = VertexAttributeToSize(attribute_bit) / sizeof(float);
        if (data.size() / component_number != vertices_number || data.size() % component_number != 0)
        {
            MEOW_ERROR("Input attributes {} data size {} mismatch with vertices number {} in factory",
                       to_string(attribute_bit),
                       data.size(),
                       vertices_number);
            return;
        }

        attributes_map[attribute_bit] = data;
    }

    std::vector<float> GeometryFactory::GetVertices(const std::vector<Meow::VertexAttributeBit>& attribute_bits)
    {
        // If last attribute bits is different with input attribute bits,
        // output vertices buffer directly.
        if (attribute_bits.size() == last_attribute_bits.size())
        {
            bool is_same = true;
            for (uint32_t i = 0; i < attribute_bits.size(); i++)
            {
                if (attribute_bits[i] != last_attribute_bits[i])
                {
                    is_same = false;
                    break;
                }
            }
            if (is_same)
            {
                return vertices_buffer;
            }
        }

        // validation existence
        for (uint32_t i = 0; i < attribute_bits.size(); i++)
        {
            if (attributes_map.find(attribute_bits[i]) == attributes_map.end())
            {
                MEOW_ERROR("Input attribute bit {} doesn't exist in factory!", to_string(attribute_bits[i]));
                return {};
            }
        }

        // Check for duplicate attributes
        {
            std::unordered_set<VertexAttributeBit> unique_attributes;
            for (const auto& attr : attribute_bits)
            {
                if (!unique_attributes.insert(attr).second)
                {
                    MEOW_ERROR("Duplicate attribute {} found in input attribute bits!", to_string(attr));
                    return {};
                }
            }
        }

        last_attribute_bits = attribute_bits;

        uint32_t vertices_data_size = 0;
        for (uint32_t i = 0; i < attribute_bits.size(); i++)
        {
            vertices_data_size += attributes_map[attribute_bits[i]].size();
        }
        vertices_buffer.resize(vertices_data_size);

        uint32_t vertices_data_size_unit = 0;
        vertices_data_size_unit          = vertices_data_size / vertices_number;

        std::vector<uint32_t> component_numbers(attribute_bits.size(), 0);
        std::vector<uint32_t> component_numbers_acc(attribute_bits.size(), 0);
        for (uint32_t i = 0; i < attribute_bits.size(); i++)
        {
            component_numbers[i] = VertexAttributeToSize(attribute_bits[i]) / sizeof(float);
            if (i > 0)
            {
                component_numbers_acc[i] = component_numbers_acc[i - 1] + component_numbers[i - 1];
            }
        }

        for (uint32_t i = 0; i < vertices_number; i++)
        {
            // according to binding order
            for (size_t j = 0; j < attribute_bits.size(); ++j)
            {
                for (uint32_t k = 0; k < component_numbers[j]; k++)
                {
                    vertices_buffer[i * vertices_data_size_unit + component_numbers_acc[j] + k] =
                        attributes_map[attribute_bits[j]][i * component_numbers[j] + k];
                }
            }
        }

        return vertices_buffer;
    }

    void GeometryFactory::clear()
    {
        vertices_number = 0;
        position.clear();
        indices.clear();
        attributes_map.clear();
        vertices_buffer.clear();
        last_attribute_bits.clear();
    }
} // namespace Meow
