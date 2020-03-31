#include "Heap.h"
#include "ConstantBuffer.h"

namespace Gpu::dx12
{

void Heap::Initialize(const ComPtr<ID3D12Device> &device,
                      D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT count)
{
    {
        D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {
            .Type = heapType,
            .NumDescriptors = count,
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        };
        ThrowIfFailed(device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_heap)));
    }

    m_descriptorSize = device->GetDescriptorHandleIncrementSize(heapType);
    m_cpuHandle = m_heap->GetCPUDescriptorHandleForHeapStart();
    m_gpuHandle = m_heap->GetGPUDescriptorHandleForHeapStart();
}

} // namespace Gpu::dx12
