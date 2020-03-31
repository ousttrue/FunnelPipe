#include "FrameTexture.h"

namespace framedata
{

std::shared_ptr<FrameTexture> FrameTexture::White()
{
    static FrameTexturePtr s_texture;
    if(!s_texture)
    {
        s_texture = std::make_shared<FrameTexture>();
        s_texture->Name = "white";
        s_texture->Image = FrameImage::White();
    }
    return s_texture;
}

} // namespace framedata
