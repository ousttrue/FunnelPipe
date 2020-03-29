#pragma once
#include <memory>
#include <string>
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
    ShaderWatcherPtr shader;
    AlphaMode alphaMode{};
    float alphaCutoff = 0;
    FrameImagePtr colorImage;
};
using FrameMaterialPtr = std::shared_ptr<FrameMaterial>;

} // namespace framedata
