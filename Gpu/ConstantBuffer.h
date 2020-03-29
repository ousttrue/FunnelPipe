#pragma once
#include "Helper.h"
#include <stdint.h>
#include <vector>

namespace Gpu::dx12
{

class ConstantBuffer : NonCopyable
{
    //
    // CPU
    //
    UINT8 *m_pCbvDataBegin = nullptr;
    std::vector<std::pair<UINT, UINT>> m_ranges;

    //
    // GPU
    //
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;

public:
    UINT Count() const { return (UINT)m_ranges.size(); }
    std::pair<UINT, UINT> Range(UINT index) const { return m_ranges[index]; }
    void Initialize(const Microsoft::WRL::ComPtr<ID3D12Device> &device, uint32_t size);
    void Assign(const std::pair<UINT, UINT> *range, uint32_t count);
    void AllRange();

    const Microsoft::WRL::ComPtr<ID3D12Resource> &Resource() const { return m_resource; }

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

} // namespace Gpu::dx12
