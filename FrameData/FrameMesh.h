#pragma once
#include <vector>
#include <memory>
#include <stdint.h>
#include <DirectXMath.h>
#include <ranges>
#include "FrameMaterial.h"

namespace framedata
{

struct FrameSubmesh
{
    uint32_t drawOffset = 0;
    uint32_t drawCount = 0;
    FrameMaterialPtr material;
};

class FrameMesh
{
public:
    std::string name;
    FrameMesh(const std::string &n)
        : name(n)
    {
    }

    static std::shared_ptr<FrameMesh> Create(const std::string &name);
    static std::shared_ptr<FrameMesh> CreateDynamic(const std::string &name,
                                                    uint32_t vertexReserve, uint32_t vertexStride,
                                                    uint32_t indexReserve, uint32_t indexStride);

    std::shared_ptr<class VertexBuffer> vertices;
    std::shared_ptr<class VertexBuffer> indices;

    std::vector<FrameSubmesh> submeshes;
    void AddSubmesh(const std::shared_ptr<FrameMesh> &mesh);
    bool Validate();
};
using FrameMeshPtr = std::shared_ptr<FrameMesh>;

} // namespace framedata
