#include "model_mesh.h"

#include "pch.h"

namespace Meow
{
    void ModelMesh::BindOnly(const vk::raii::CommandBuffer& cmd_buffer)
    {
        FUNCTION_TIMER();

        for (int i = 0; i < primitives.size(); ++i)
        {
            primitives[i]->BindOnly(cmd_buffer);
        }
    }

    void ModelMesh::DrawOnly(const vk::raii::CommandBuffer& cmd_buffer)
    {
        FUNCTION_TIMER();

        for (int i = 0; i < primitives.size(); ++i)
        {
            primitives[i]->DrawOnly(cmd_buffer);
        }
    }

    void ModelMesh::BindDrawCmd(const vk::raii::CommandBuffer& cmd_buffer)
    {
        FUNCTION_TIMER();

        for (int i = 0; i < primitives.size(); ++i)
        {
            primitives[i]->BindDrawCmd(cmd_buffer);
        }
    }
}; // namespace Meow