#include "FrameImage.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <sstream>

namespace framedata
{

std::shared_ptr<FrameImage> FrameImage::Create()
{
    return FrameImagePtr(new FrameImage);
}

std::shared_ptr<FrameImage> FrameImage::CreateRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    FrameImagePtr image;
    // if (!s_image)
    {
        image = FrameImagePtr(new FrameImage);
        std::stringstream ss;
        ss << "(" << r << ", " << g << ", " << b << ", " << a << ")";
        image->name = ss.str();
        image->width = 64;
        image->height = 64;
        image->buffer.resize(image->width * image->height * 4);
        for (auto it = image->buffer.begin(); it != image->buffer.end();)
        {
            *(it++) = r;
            *(it++) = g;
            *(it++) = b;
            *(it++) = a;
        }
    }
    return image;
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
