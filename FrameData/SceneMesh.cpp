#include "SceneMesh.h"
#include "VertexBuffer.h"
#include "Shader.h"
#include <algorithm>

namespace framedata
{

SceneMeshPtr SceneMesh::Create(const std::wstring &name)
{
    return SceneMeshPtr(new SceneMesh(name));
}

std::shared_ptr<SceneMesh> SceneMesh::CreateDynamic(
    const std::wstring &name,
    uint32_t vertexReserve, uint32_t vertexStride,
    uint32_t indexReserve, uint32_t indexStride)
{
    auto mesh = SceneMeshPtr(new SceneMesh(name));

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

void SceneMesh::AddSubmesh(const std::shared_ptr<SceneMesh> &mesh)
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
    submeshes.push_back(mesh->submeshes.front());
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

bool SceneMesh::Validate()
{
    for (auto &submesh : submeshes)
    {
        auto shader = submesh.material->shader->Compiled();
        if (shader->Generation() < 0)
        {
            return false;
        }

        // auto resource = CreateResourceItem(device, m_uploader, sceneMesh, shader->inputLayout(), shader->inputLayoutCount());
        auto dstStride = 0;
        int inputLayoutCount;
        auto inputLayout = shader->inputLayout(&inputLayoutCount);
        for (int i = 0; i < inputLayoutCount; ++i)
        {
            dstStride += GetStride(inputLayout[i].Format);
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