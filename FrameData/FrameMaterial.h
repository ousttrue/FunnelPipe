#pragma once
#include <memory>
#include <string>
#include <array>
#include <stdint.h>
#include "FrameImage.h"
#include "ShaderWatcher.h"

namespace framedata
{

enum class AlphaMode
{
    Opaque,
    Mask,
    Blend,
};

class FrameMaterial
{

public:
    static std::shared_ptr<FrameMaterial> Create();

    std::string name;
    ShaderWatcherPtr shaderSource;
    AlphaMode alphaMode{};
    float alphaCutoff = 0;
    FrameImagePtr colorImage;
    std::array<float, 4> color = {1, 1, 1, 1};
};
using FrameMaterialPtr = std::shared_ptr<FrameMaterial>;

} // namespace framedata
