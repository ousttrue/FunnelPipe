#pragma once
#include <memory>
#include <string>
#include <array>
#include <stdint.h>
#include "FrameTexture.h"
#include "ShaderPass.h"

namespace framedata
{

enum class AlphaMode
{
    Opaque,
    Mask,
    Blend,
};

struct FrameMaterial
{
    std::string Name;

    ShaderPassPtr Shader;

    AlphaMode AlphaMode{};
    float AlphaCutoff = 0;

    bool DoubleSided = false;

    FrameTexturePtr ColorTexture;
    std::array<float, 4> Color = {1, 1, 1, 1};

    FrameTexturePtr NormalTexture;
    FrameTexturePtr OcclusionTexture;

    FrameTexturePtr EmissiveTexture;
    std::array<float, 3> Emissive = {0, 0, 0};

    FrameTexturePtr MetallicRoughnessTexture;
    float Metallic = 1.0f;
    float Roughness = 1.0f;
};
using FrameMaterialPtr = std::shared_ptr<FrameMaterial>;

} // namespace framedata
