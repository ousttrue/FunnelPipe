#pragma once
#include "Helper.h"
#include <memory>
#include <vector>
#include <functional>

namespace Gpu::dx12
{

class Mesh : NonCopyable
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::shared_ptr<class ResourceItem> m_vertexBuffer;
    std::shared_ptr<class ResourceItem> m_indexBuffer;

public:
    void VertexBuffer(const std::shared_ptr<class ResourceItem> &item) { m_vertexBuffer = item; }
    const std::shared_ptr<class ResourceItem> &VertexBuffer() const { return m_vertexBuffer; }
    void IndexBuffer(const std::shared_ptr<class ResourceItem> &item) { m_indexBuffer = item; }
    const std::shared_ptr<class ResourceItem> &IndexBuffer() const { return m_indexBuffer; }
    std::pair<bool, std::function<void()>> IsDrawable(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

} // namespace Gpu::dx12
