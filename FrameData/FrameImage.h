#pragma once
#include <memory>
#include <vector>
#include <string>
#include <stdint.h>

namespace framedata
{

enum class ImageType
{
    Unknown,
    Raw,
};

class FrameImage
{
public:
    // empty
    static std::shared_ptr<FrameImage> Create();

    // default 2x2 white
    static std::shared_ptr<FrameImage> White();

    // load
    static std::shared_ptr<FrameImage> Load(const uint8_t *p, int size);

    std::vector<uint8_t> buffer;
    // ImageType type = ImageType::Unknown;
    int width = 0;
    int height = 0;
    std::string name;

    uint32_t size() const
    {
        return width * height * 4;
    }

    void SetRawBytes(const uint8_t *p, int w, int h)
    {
        // type = ImageType::Raw;
        width = w;
        height = h;
        buffer.assign(p, p + w * h);
    }
};
using FrameImagePtr = std::shared_ptr<FrameImage>;

} // namespace framedata
