#pragma once
#include "FrameMesh.h"
#include "VertexBuffer.h"
#include "DirectoryWatcher.h"
#include "Shader.h"
#include <array>

namespace framedata
{

///
/// 無限グリッド
///
static std::shared_ptr<FrameMesh> CreateGrid()
{
    struct GridVertex
    {
        std::array<float, 2> position;
        std::array<float, 2> uv;
    };
    GridVertex vertices[] = {
        {{-1, 1}, {0, 0}},
        {{-1, -1}, {0, 1}},
        {{1, -1}, {1, 1}},
        {{1, 1}, {1, 0}},
    };
    uint16_t indices[] = {
        0, 1, 2, //
        2, 3, 0, //
    };
    auto mesh = FrameMesh::Create("grid");
    mesh->vertices = VertexBuffer::CreateStatic(
        Semantics::Vertex,
        sizeof(vertices[0]), vertices, sizeof(vertices));
    mesh->indices = VertexBuffer::CreateStatic(
        Semantics::Index,
        2, indices, sizeof(indices));
    {
        auto material = std::make_shared<FrameMaterial>();
        material->shader = std::make_shared<Shader>("grid.hlsl");
        material->shader->Compile(DirectoryWatcher::Instance().Get(L"grid.hlsl")->String());
        material->alphaMode = AlphaMode::Blend;
        mesh->submeshes.push_back({.drawCount = _countof(indices),
                                   .material = material});
    }
    return mesh;
}

} // namespace framedata
