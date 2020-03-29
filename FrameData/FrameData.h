#pragma once
#include "VertexBuffer.h"
#include "Shader.h"
#include "ShaderManager.h"
#include "MeshFactory/CreateGrid.h"
#include "SceneMesh.h"

#include <array>
#include <vector>
#include <memory>
#include "ShaderConstantVariable.h"
#include "SceneMesh.h"

namespace framedata
{
using namespace hierarchy;

struct CBValue
{
    ConstantSemantics semantic;
    const void *p;
    uint32_t size;
};

// each View
// https://gamedev.stackexchange.com/questions/105572/c-struct-doesnt-align-correctly-to-a-pixel-shader-cbuffer
#pragma pack(push)
#pragma pack(16)
struct ViewConstants
{
    std::array<float, 16> b0View;
    std::array<float, 16> b0Projection;
    std::array<float, 3> b0LightDir;
    float _padding0;
    std::array<float, 3> b0LightColor;
    float _padding1;
    std::array<float, 3> b0CameraPosition;
    float _padding2;
    std::array<float, 2> b0ScreenSize;
    float fovY;
    float _padding3;
    // DirectX::XMFLOAT4X4 b0ViewInv;
};
#pragma pack(pop)
static_assert(sizeof(ViewConstants) == 16 * 12, "sizeof ViewConstantsSize");

struct FrameData
{
    size_t ViewID;
    uint32_t ViewWidth() const { return (uint32_t)ViewConstantBuffer.b0ScreenSize[0]; }
    uint32_t ViewHeight() const { return (uint32_t)ViewConstantBuffer.b0ScreenSize[1]; }
    // std::vector<uint8_t> ViewConstantBuffer;
    // int Width = 0;
    // int Height = 0;
    // std::array<float, 16> Projection = {};
    // std::array<float, 16> View = {};
    // std::array<float, 3> CameraPosition = {0, 0, 0};
    // float CameraFovYRadians = 1.0f;
    std::array<float, 4> ViewClearColor = {0, 0, 0, 1};
    bool ShowGrid = true;
    bool ShowGizmo = true;
    bool ShowVR = false;

    ViewConstants ViewConstantBuffer;

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

} // namespace framedata
