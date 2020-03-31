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
    if (!texture)
    {
        return PushTexture(FrameTexture::Zero());
    }

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

    SRVViews.push_back(SRVView(
        (uint16_t)PushTexture(material->ColorTexture),
        (uint16_t)PushTexture(FrameTexture::Zero()),
        (uint16_t)PushTexture(FrameTexture::Zero()),
        (uint16_t)PushTexture(FrameTexture::Zero()),
        (uint16_t)PushTexture(FrameTexture::Zero()),
        (uint16_t)PushTexture(FrameTexture::Zero()),
        (uint16_t)PushTexture(FrameTexture::Zero()),
        (uint16_t)PushTexture(FrameTexture::Zero())));

    if (material->Shader == ShaderManager::Instance().GltfPBR())
    {
        // Texture2D baseColourTexture : register(t0);
        // Texture2D normalTexture : register(t1);
        if (material->NormalTexture)
        {
            SRVViews.back().SRV1TextureIndex = (uint16_t)PushTexture(material->NormalTexture);
        }
        // Texture2D emissionTexture : register(t2);
        if (material->EmissiveTexture)
        {
            SRVViews.back().SRV2TextureIndex = (uint16_t)PushTexture(material->EmissiveTexture);
        }
        // Texture2D occlusionTexture : register(t3);
        if (material->OcclusionTexture)
        {
            SRVViews.back().SRV3TextureIndex = (uint16_t)PushTexture(material->OcclusionTexture);
        }
        // Texture2D metallicRoughnessTexture : register(t4);
        if (material->MetallicRoughnessTexture)
        {
            SRVViews.back().SRV4TextureIndex = (uint16_t)PushTexture(material->MetallicRoughnessTexture);
        }
        // TextureCube envDiffuseTexture : register(t5);
        SRVViews.back().SRV5TextureIndex = (uint16_t)PushTexture(FrameTexture::Cube());
        // Texture2D brdfLutTexture : register(t6);
        // TextureCube envSpecularTexture : register(t7);
        SRVViews.back().SRV7TextureIndex = (uint16_t)PushTexture(FrameTexture::Cube());
        auto a = 0;
    }

    MaterialMap.insert(std::make_pair(material, index));
    return index;
}

} // namespace framedata
