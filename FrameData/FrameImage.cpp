#include "FrameImage.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace framedata
{

std::shared_ptr<FrameImage> FrameImage::Create()
{
    return FrameImagePtr(new FrameImage);
}

std::shared_ptr<FrameImage> FrameImage::White()
{
    static FrameImagePtr s_image;
    if (!s_image)
    {
        s_image = FrameImagePtr(new FrameImage);
        s_image->name = "(white)";
        s_image->width = 64;
        s_image->height = 64;
        s_image->buffer.resize(s_image->width * s_image->height * 4, 255);
    }
    return s_image;
}

std::shared_ptr<FrameImage> FrameImage::Load(const uint8_t *p, int size)
{
    int x, y, n;
    unsigned char *data = stbi_load_from_memory(p, size, &x, &y, &n, 4);
    if (!data)
    {
        return nullptr;
    }

    auto image = Create();
    image->width = x;
    image->height = y;
    image->buffer.assign(data, data + x * y * 4);
    stbi_image_free(data);
    return image;
}

} // namespace framedata
