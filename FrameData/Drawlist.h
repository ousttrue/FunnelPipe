#pragma once
#include <array>
#include <vector>
#include <memory>
#include "ShaderConstantVariable.h"
#include "SceneMesh.h"

namespace hierarchy
{

struct CBValue
{
    ConstantSemantics semantic;
    const void *p;
    uint32_t size;
};

struct DrawList
{
    //
    // 可変サイズのCBバッファの配列
    // TODO: 16byte(256?) alignment
    //
    std::vector<uint8_t> CB;
    std::vector<std::pair<uint32_t, uint32_t>> CBRanges;

    std::pair<uint32_t, uint32_t> PushCB(const ConstantBuffer *cb, const CBValue *value, int count);

    struct Buffer
    {
        uint8_t *Ptr;
        uint32_t Size;
        uint32_t Stride;
    };
    struct DrawItem
    {
        // mesh
        std::shared_ptr<SceneMesh> Mesh;
        Buffer Vertices{};
        Buffer Indices{};
        hierarchy::SceneSubmesh Submesh;
    };
    std::vector<DrawItem> Items;

    void Clear()
    {
        CB.clear();
        CBRanges.clear();
        Items.clear();
    }
};

} // namespace hierarchy
