#pragma once
#include <memory>
#include <string>
#include <array>
#include <stdint.h>
#include "FrameImage.h"
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
    FrameImagePtr ColorImage;
    std::array<float, 4> Color = {1, 1, 1, 1};
};
using FrameMaterialPtr = std::shared_ptr<FrameMaterial>;

} // namespace framedata
