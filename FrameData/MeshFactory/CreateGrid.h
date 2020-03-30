#pragma once
#include "VertexBuffer.h"
#include "ShaderManager.h"
#include "FrameMesh.h"
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
        auto material = FrameMaterial::Create();
        material->shaderSource = ShaderManager::Instance().GetSource("grid.hlsl");
        material->alphaMode = AlphaMode::Blend;
        mesh->submeshes.push_back({.drawCount = _countof(indices),
                                   .material = material});
    }
    return mesh;
}

} // namespace framedata
