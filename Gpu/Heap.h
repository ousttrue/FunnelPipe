#pragma once
#include "Helper.h"
#include <vector>

namespace Gpu::dx12
{

class Heap : NonCopyable
{
    // CBV_SRV_UAV
    ComPtr<ID3D12DescriptorHeap> m_heap;
    UINT m_descriptorSize = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE m_cpuHandle{};
    D3D12_GPU_DESCRIPTOR_HANDLE m_gpuHandle{};

public:
    ID3D12DescriptorHeap *Get() { return m_heap.Get(); }
    D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle(int index) const { return {m_cpuHandle.ptr + m_descriptorSize * index}; }
    D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle(int index) const { return {m_gpuHandle.ptr + m_descriptorSize * index}; }
    void Initialize(const ComPtr<ID3D12Device> &device,
                    D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT count);
};

} // namespace Gpu::dx12
