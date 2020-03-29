#pragma once
#include "Helper.h"
#include <stdint.h>
#include <vector>
// #include <DrawList.h>

namespace Gpu::dx12
{

class ConstantBufferBase : NonCopyable
{
    //
    // CPU
    //
protected:
    std::vector<uint8_t> m_bytes;
    UINT8 *m_pCbvDataBegin = nullptr;

public:
    virtual UINT Count() const = 0;
    virtual std::pair<UINT, UINT> Range(UINT index) const = 0;

    //
    // GPU
    //
protected:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

public:
    const Microsoft::WRL::ComPtr<ID3D12Resource> &Resource() const { return m_resource; }

    void CopyToGpu()
    {
        memcpy(m_pCbvDataBegin, m_bytes.data(), m_bytes.size());
    }

    void CopyToGpu(const void *p, size_t size)
    {
        memcpy(m_pCbvDataBegin, p, size);
    }

    template <typename T>
    void CopyToGpu(const T &t)
    {
        CopyToGpu(&t, sizeof(T));
    }
};

///
/// 可変長のバッファ
///
class SemanticsConstantBuffer : public ConstantBufferBase
{
    UINT m_allocSizePerItem;
    std::vector<std::pair<UINT, UINT>> m_ranges;

public:
    SemanticsConstantBuffer(UINT allocSizePerItem)
        : m_allocSizePerItem(allocSizePerItem)
    {
    }

    UINT Count() const override { return (UINT)m_ranges.size(); }
    std::pair<UINT, UINT> Range(UINT index) const override { return m_ranges[index]; }

    void Initialize(const Microsoft::WRL::ComPtr<ID3D12Device> &device, int count);
    void Assign(const std::pair<UINT, UINT> *range, uint32_t count);
    void AllRange();
};

} // namespace Gpu::dx12
