#include "FrameTexture.h"

namespace framedata
{

std::shared_ptr<FrameTexture> FrameTexture::One()
{
    static FrameTexturePtr s_texture;
    if (!s_texture)
    {
        s_texture = std::make_shared<FrameTexture>();
        s_texture->Name = "white";
        s_texture->Images.push_back(FrameImage::CreateRGBA(255, 255, 255, 255));
    }
    return s_texture;
}

std::shared_ptr<FrameTexture> FrameTexture::Zero()
{
    static FrameTexturePtr s_texture;
    if (!s_texture)
    {
        s_texture = std::make_shared<FrameTexture>();
        s_texture->Name = "zero";
        s_texture->Images.push_back(FrameImage::CreateRGBA(0, 0, 0, 0));
    }
    return s_texture;
}

std::shared_ptr<FrameTexture> FrameTexture::Cube()
{
    static FrameTexturePtr s_texture;
    if (!s_texture)
    {
        s_texture = std::make_shared<FrameTexture>();
        s_texture->Name = "cube";
        s_texture->Images.push_back(FrameImage::CreateRGBA(255, 0, 0, 255));
        s_texture->Images.push_back(FrameImage::CreateRGBA(0, 255, 0, 255));
        s_texture->Images.push_back(FrameImage::CreateRGBA(0, 0, 255, 255));
        s_texture->Images.push_back(FrameImage::CreateRGBA(0, 255, 255, 255));
        s_texture->Images.push_back(FrameImage::CreateRGBA(255, 0, 255, 255));
        s_texture->Images.push_back(FrameImage::CreateRGBA(255, 255, 0, 255));
    }
    return s_texture;
}

} // namespace framedata
