#pragma once
#include "Helper.h"
#include <memory>
#include <functional>

namespace Gpu::dx12
{

class Texture : NonCopyable
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    std::shared_ptr<class ResourceItem> m_imageBuffer;

    const ComPtr<ID3D12Resource> &Resource() const;

    void ImageBuffer(const std::shared_ptr<class ResourceItem> &item)
    {
        m_imageBuffer = item;
    }

    std::pair<bool, std::function<void()>> IsDrawable(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

} // namespace Gpu::dx12