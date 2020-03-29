#pragma once
#include <vector>
#include <memory>
#include <stdint.h>
#include <DirectXMath.h>
#include <ranges>
#include "SceneMaterial.h"

namespace hierarchy
{

struct SceneSubmesh
{
    uint32_t drawOffset = 0;
    uint32_t drawCount = 0;
    SceneMaterialPtr material;
};

class SceneMesh
{
public:
    std::wstring name;
    SceneMesh(const std::wstring &n)
        : name(n)
    {
    }

    static std::shared_ptr<SceneMesh> Create(const std::wstring &name);
    static std::shared_ptr<SceneMesh> CreateDynamic(const std::wstring &name,
                                                    uint32_t vertexReserve, uint32_t vertexStride,
                                                    uint32_t indexReserve, uint32_t indexStride);

    std::shared_ptr<class VertexBuffer> vertices;
    std::shared_ptr<class VertexBuffer> indices;

    std::vector<SceneSubmesh> submeshes;
    void AddSubmesh(const std::shared_ptr<SceneMesh> &mesh);
    bool Validate();
};
using SceneMeshPtr = std::shared_ptr<SceneMesh>;

} // namespace hierarchy
