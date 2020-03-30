#pragma once
#include "VertexBuffer.h"
#include "Shader.h"
#include "MeshFactory/CreateGrid.h"
#include "FrameMesh.h"
#include "ToUnicode.h"
#include "DirectoryWatcher.h"
#include "ShaderManager.h"

#include <array>
#include <vector>
#include <memory>
#include "ShaderConstantVariable.h"
#include "FrameMesh.h"

namespace framedata
{

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
};
#pragma pack(pop)
static_assert(sizeof(ViewConstants) == 16 * 12, "sizeof ViewConstantsSize");

struct FrameData
{
    size_t ViewID;
    ViewConstants ViewConstantBuffer;
    uint32_t ViewWidth() const { return (uint32_t)ViewConstantBuffer.b0ScreenSize[0]; }
    uint32_t ViewHeight() const { return (uint32_t)ViewConstantBuffer.b0ScreenSize[1]; }
    std::array<float, 4> ViewClearColor = {0, 0, 0, 1};
    bool ShowGrid = true;
    bool ShowGizmo = true;
    bool ShowVR = false;

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
    struct MeshItem
    {
        std::shared_ptr<FrameMesh> Mesh;
        Buffer Vertices{};
        Buffer Indices{};
        Buffer Skin{};
    };
    std::vector<MeshItem> Meshlist;

    // texture の slot 割り当て
    std::vector<FrameImagePtr> Textures;
    std::unordered_map<FrameImagePtr, size_t> TextureMap;
    size_t PushImage(const FrameImagePtr &texture)
    {
        auto found = TextureMap.find(texture);
        if (found != TextureMap.end())
        {
            return found->second;
        }
        auto index = Textures.size();
        Textures.push_back(texture);
        TextureMap.insert(std::make_pair(texture, index));
        return index;
    }

    // material毎の slot 割り当て
    struct SRVView
    {
        uint16_t SRV0TextureIndex;
        uint16_t SRV1TextureIndex;
        uint16_t SRV2TextureIndex;
        uint16_t SRV3TextureIndex;
        uint16_t SRV4TextureIndex;
        uint16_t SRV5TextureIndex;
        uint16_t SRV6TextureIndex;
        uint16_t SRV7TextureIndex;
    };
    std::vector<SRVView> SRVViews;
    // std::vector<FrameMaterialPtr> Materials;
    std::unordered_map<FrameMaterialPtr, size_t> MaterialMap;
    size_t PushMaterial(const framedata::FrameMaterialPtr &material)
    {
        auto found = MaterialMap.find(material);
        if (found != MaterialMap.end())
        {
            return found->second;
        }
        auto index = SRVViews.size();

        auto white = PushImage(FrameImage::White());

        SRVViews.push_back({
            .SRV0TextureIndex = (uint16_t)(material->ColorImage ? PushImage(material->ColorImage) : white),
            .SRV1TextureIndex = (uint16_t)white,
            .SRV2TextureIndex = (uint16_t)white,
            .SRV3TextureIndex = (uint16_t)white,
            .SRV4TextureIndex = (uint16_t)white,
            .SRV5TextureIndex = (uint16_t)white,
            .SRV6TextureIndex = (uint16_t)white,
            .SRV7TextureIndex = (uint16_t)white,
        });
        MaterialMap.insert(std::make_pair(material, index));
        return index;
    }

    struct DrawItem
    {
        std::shared_ptr<FrameMesh> Mesh;
        FrameSubmesh Submesh;
        uint32_t MaterialIndex;
    };
    // CBRanges.size() == Drawlist.size()
    std::vector<DrawItem> Drawlist;
    void PushDraw(const FrameMeshPtr &mesh, uint32_t submeshIndex)
    {
        PushDraw(mesh, mesh->submeshes[submeshIndex]);
    }
    void PushDraw(const FrameMeshPtr &mesh, const FrameSubmesh &submesh)
    {
        auto materialIndex = (uint32_t)PushMaterial(submesh.material);
        Drawlist.push_back({
            .Mesh = mesh,
            .Submesh = submesh,
            .MaterialIndex = materialIndex,
        });
    }

    void Clear()
    {
        Meshlist.clear();

        CB.clear();
        CBRanges.clear();
        Drawlist.clear();

        Textures.clear();
        TextureMap.clear();
        MaterialMap.clear();
        SRVViews.clear();
    }
};

} // namespace framedata
