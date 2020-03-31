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
    std::vector<FrameImagePtr> Images;

    static std::shared_ptr<FrameTexture> One();
    static std::shared_ptr<FrameTexture> Zero();
    static std::shared_ptr<FrameTexture> Cube();
};
using FrameTexturePtr = std::shared_ptr<FrameTexture>;

} // namespace framedata
