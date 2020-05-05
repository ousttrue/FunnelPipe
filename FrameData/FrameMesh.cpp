#include "FrameMesh.h"
#include "VertexBuffer.h"
#include "ShaderPass.h"
#include <algorithm>

namespace framedata
{

FrameMeshPtr FrameMesh::Create(const std::string &name)
{
    return FrameMeshPtr(new FrameMesh(name));
}

std::shared_ptr<FrameMesh> FrameMesh::CreateDynamic(
    const std::string &name,
    uint32_t vertexReserve, uint32_t vertexStride,
    uint32_t indexReserve, uint32_t indexStride)
{
    auto mesh = FrameMeshPtr(new FrameMesh(name));

    mesh->vertices = VertexBuffer::CreateDynamic(
        Semantics::Vertex,
        vertexStride,
        vertexReserve);

    mesh->indices = VertexBuffer::CreateDynamic(
        Semantics::Index,
        indexStride,
        indexReserve);

    return mesh;
}

void FrameMesh::AddSubmesh(const std::shared_ptr<FrameMesh> &mesh)
{
    if (!vertices)
    {
        vertices = VertexBuffer::CreateStatic(Semantics::Vertex, mesh->vertices->stride, nullptr, 0);
    }
    if (!indices)
    {
        indices = VertexBuffer::CreateStatic(Semantics::Index, mesh->indices->stride, nullptr, 0);
    }

    auto indexOffset = vertices->Count();

    auto src = mesh->vertices;
    {
        bool found = false;
        // auto dst = vertices;
        {
            if (src->semantic == vertices->semantic)
            {
                found = true;
                auto sum = vertices->Count() + src->Count();
                vertices->Append(src);
                assert(vertices->Count() == sum);
            }
            else
            {
                throw;
            }
        }
    }

    auto last = indices->Count();
    indices->Append(mesh->indices);
    if (indices->stride == 2)
    {
        auto src = (uint16_t *)mesh->indices->buffer.data();
        auto dst = (uint16_t *)indices->buffer.data() + last;
        auto count = mesh->indices->Count();
        for (size_t i = 0; i < count; ++i, ++src, ++dst)
        {
            *dst = *src + indexOffset;
        }
    }
    else if (indices->stride == 4)
    {
        auto src = (uint32_t *)mesh->indices->buffer.data();
        auto dst = (uint32_t *)indices->buffer.data() + last;
        auto count = mesh->indices->Count();
        for (size_t i = 0; i < count; ++i, ++src, ++dst)
        {
            *dst = *src + indexOffset;
        }
    }
    else
    {
        throw;
    }

    if (mesh->submeshes.size() != 1)
    {
        throw;
    }
    auto offset = submeshes.empty()
                      ? 0
                      : submeshes.back().drawOffset + submeshes.back().drawCount;
    submeshes.push_back(mesh->submeshes.front());
    submeshes.back().drawOffset = offset;
}

static int GetStride(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return 4;

    case DXGI_FORMAT_R32G32_FLOAT:
        return 8;

    case DXGI_FORMAT_R32G32B32_FLOAT:
        return 12;

    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return 16;
    }

    throw;
}

bool FrameMesh::Validate()
{
    for (auto &submesh : submeshes)
    {
        // auto shader = submesh.material->shader;
        // if (!shader || shader->Generation() < 0)
        // {
        //     return false;
        // }

        auto dstStride = 0;
        auto inputLayout = submesh.material->Shader->VS->InputLayout();
        for (auto elm: inputLayout)
        {
            dstStride += GetStride(elm.Format);
        }

        if (vertices->stride != dstStride)
        {
            // LOGE << "buffer stride difference with shader stride";
            return false;
        }
    }
    return true;
}

} // namespace framedata