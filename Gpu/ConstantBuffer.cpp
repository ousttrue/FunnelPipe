#include "ConstantBuffer.h"

namespace Gpu::dx12
{

void ConstantBuffer::Initialize(const Microsoft::WRL::ComPtr<ID3D12Device> &device,
                                uint32_t size)
{
    D3D12_HEAP_PROPERTIES prop{
        .Type = D3D12_HEAP_TYPE_UPLOAD,
    };
    D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };
    ThrowIfFailed(device->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)));

    // Map and initialize the constant buffer. We don't unmap this until the
    // app closes. Keeping things mapped for the lifetime of the resource is okay.
    D3D12_RANGE readRange{0, 0}; // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void **>(&m_pCbvDataBegin)));
}

void ConstantBuffer::Assign(const std::pair<UINT, UINT> *range, uint32_t count)
{
    if (count == 0)
    {
        m_ranges.clear();
        return;
    }
    m_ranges.assign(range, range + count);
}

void ConstantBuffer::AllRange()
{
    D3D12_RESOURCE_DESC desc = m_resource->GetDesc();
    m_ranges.push_back({0, desc.Width});
}

} // namespace Gpu::dx12
