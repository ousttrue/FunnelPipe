#pragma once
#include <memory>
#include <string>
#include <array>
#include <stdint.h>
#include "FrameImage.h"
#include "Shader.h"

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
    std::string name;
    ShaderPtr shader;
    AlphaMode alphaMode{};
    float alphaCutoff = 0;
    FrameImagePtr colorImage;
    std::array<float, 4> color = {1, 1, 1, 1};
};
using FrameMaterialPtr = std::shared_ptr<FrameMaterial>;

} // namespace framedata
