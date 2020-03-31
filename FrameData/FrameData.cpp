#include "FrameData.h"

namespace framedata
{

std::pair<uint32_t, uint32_t> FrameData::PushCB(const ConstantBuffer *cb, const CBValue *value, int count)
{
    auto offset = (uint32_t)CB.size();
    if (cb)
    {
        CBRanges.push_back({offset, cb->End()});
        CB.resize(CB.size() + CBRanges.back().second);
        auto p = CB.data() + offset;
        for (int i = 0; i < count; ++i, ++value)
        {
            for (auto &var : cb->Variables)
            {
                if (var.Semantic == value->semantic)
                {
                    // copy value
                    memcpy(p + var.Offset, value->p, value->size);
                    break;
                }
            }
        }
    }
    else
    {
        // 空の場合
        CBRanges.push_back({offset, 256});
        CB.resize(CB.size() + CBRanges.back().second);
    }

    return CBRanges.back();
}

size_t FrameData::PushTexture(const FrameTexturePtr &texture)
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

size_t FrameData::PushMaterial(const framedata::FrameMaterialPtr &material)
{
    auto found = MaterialMap.find(material);
    if (found != MaterialMap.end())
    {
        return found->second;
    }
    auto index = SRVViews.size();

    auto white = PushTexture(FrameTexture::White());
    auto cube = PushTexture(FrameTexture::Cube());

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
        SRVViews.back().SRV5TextureIndex = (uint16_t)cube;
        // Texture2D brdfLutTexture : register(t6);
        // TextureCube envSpecularTexture : register(t7);
        SRVViews.back().SRV7TextureIndex = (uint16_t)cube;
        auto a = 0;
    }

    MaterialMap.insert(std::make_pair(material, index));
    return index;
}

} // namespace framedata
