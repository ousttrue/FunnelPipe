#pragma once
#include "FrameImage.h"
#include <memory>

namespace framedata
{

enum class TextureTypes
{
    Textue2D,
};

struct FrameTexture
{
    std::string Name;
    TextureTypes TextureType = TextureTypes::Textue2D;
    FrameImagePtr Image;

    // default 2x2 white
    static std::shared_ptr<FrameTexture> White();
};
using FrameTexturePtr = std::shared_ptr<FrameTexture>;

} // namespace framedata
