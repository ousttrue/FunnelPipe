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
    std::vector<FrameTexturePtr> Textures;
    std::unordered_map<FrameTexturePtr, size_t> TextureMap;
    size_t PushTexture(const FrameTexturePtr &texture)
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
    union SRVView {
        uint16_t list[8];
        struct
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

        SRVView(uint16_t i0, uint16_t i1, uint16_t i2, uint16_t i3,
                uint16_t i4, uint16_t i5, uint16_t i6, uint16_t i7)
            : SRV0TextureIndex(i0), SRV1TextureIndex(i1), SRV2TextureIndex(i2), SRV3TextureIndex(i3), SRV4TextureIndex(i4), SRV5TextureIndex(i5), SRV6TextureIndex(i6), SRV7TextureIndex(i7)
        {
        }
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

        auto white = PushTexture(FrameTexture::White());

        SRVViews.push_back(SRVView(
            (uint16_t)(material->ColorTexture ? PushTexture(material->ColorTexture) : white),
            (uint16_t)white,
            (uint16_t)white,
            (uint16_t)white,
            (uint16_t)white,
            (uint16_t)white,
            (uint16_t)white,
            (uint16_t)white));

        if (material->Shader == ShaderManager::Instance().GltfPBR())
        {
            // Texture2D baseColourTexture : register(t0);
            // Texture2D normalTexture : register(t1);
            // Texture2D emissionTexture : register(t2);
            // Texture2D occlusionTexture : register(t3);
            // Texture2D metallicRoughnessTexture : register(t4);
            // TextureCube envDiffuseTexture : register(t5);
            // Texture2D brdfLutTexture : register(t6);
            // TextureCube envSpecularTexture : register(t7);
            auto a = 0;
        }

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
