#include "ResourceItem.h"
#include "CommandList.h"
#include <FrameData.h>
#include <system_error>

namespace Gpu::dx12
{
ResourceItem::ResourceItem(
    const ComPtr<ID3D12Resource> &resource,
    D3D12_RESOURCE_STATES state,
    LPCWSTR name)
    : m_resource(resource)
{
    m_state.State = state;
    m_state.Upload = UploadStates::None;
    resource->SetName(name);

    m_name = UnicodeToUtf8(name);
}

void ResourceItem::MapCopyUnmap(const void *p, UINT byteLength, UINT stride)
{
    // Copy the triangle data to the vertex buffer.
    UINT8 *begin;
    D3D12_RANGE readRange{0, 0}; // We do not intend to read from this resource on the CPU.
    ThrowIfFailed(m_resource->Map(0, &readRange, reinterpret_cast<void **>(&begin)));
    memcpy(begin, p, byteLength);
    m_resource->Unmap(0, nullptr);

    m_byteLength = byteLength;
    m_stride = stride;
    m_count = byteLength / stride;
    m_state.Upload = UploadStates::Uploaded;
}

std::function<void()> ResourceItem::EnqueueTransition(const ComPtr<ID3D12GraphicsCommandList> &commandList, D3D12_RESOURCE_STATES state)
{
    m_state.Upload = UploadStates::Enqueued;
    D3D12_RESOURCE_BARRIER barrier{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = {
            .pResource = m_resource.Get(),
            .StateBefore = m_state.State,
            .StateAfter = state,
        },
    };
    commandList->ResourceBarrier(1, &barrier);

    std::weak_ptr weak = shared_from_this();
    auto callback = [weak, state]() {
        auto shared = weak.lock();
        if (shared)
        {
            shared->m_state.State = state;
            shared->m_state.Upload = UploadStates::Uploaded;
        }
    };

    return callback;
}

void ResourceItem::EnqueueUpload(CommandList *commandList,
                                 const std::shared_ptr<ResourceItem> &upload,
                                 const void *p, UINT byteLength, UINT stride)
{
    // upload
    upload->MapCopyUnmap(p, byteLength, stride);

    // copy command
    auto desc = m_resource->GetDesc();
    switch (desc.Dimension)
    {
    case D3D12_RESOURCE_DIMENSION_BUFFER:
        // TODO: footprint, offset
        commandList->Get()->CopyBufferRegion(m_resource.Get(), 0,
                                             upload->Resource().Get(), 0, byteLength);
        break;

    case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
    {
        D3D12_TEXTURE_COPY_LOCATION src{
            .pResource = upload->Resource().Get(),
            .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
            .PlacedFootprint = {
                .Offset = 0,
                .Footprint = {
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .Width = stride / 4,
                    .Height = byteLength / stride,
                    .Depth = 1,
                    .RowPitch = stride,
                }}};
        D3D12_TEXTURE_COPY_LOCATION dst{
            .pResource = m_resource.Get(),
            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            .SubresourceIndex = 0};
        commandList->Get()->CopyTextureRegion(&dst,
                                              0, 0, 0, &src, nullptr);
    }
    break;

    default:
        // not implemented
        throw;
    }

    std::weak_ptr weak = shared_from_this();
    auto callback = [weak]() {
        auto shared = weak.lock();
        if (shared)
        {
            shared->m_state.Upload = UploadStates::Uploaded;
        }
    };

    commandList->AddOnCompleted(callback);

    m_byteLength = byteLength;
    m_stride = stride;
    m_count = byteLength / stride;
}

std::shared_ptr<ResourceItem> ResourceItem::CreateDynamic(const ComPtr<ID3D12Device> &device, UINT byteLength, LPCWSTR name)
{
    D3D12_HEAP_PROPERTIES prop{
        .Type = D3D12_HEAP_TYPE_UPLOAD,
    };
    D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = byteLength,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };
    ComPtr<ID3D12Resource> resource;
    ThrowIfFailed(device->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&resource)));

    return std::shared_ptr<ResourceItem>(
        new ResourceItem(resource, D3D12_RESOURCE_STATE_GENERIC_READ, name));
}

std::shared_ptr<ResourceItem> ResourceItem::CreateStatic(const ComPtr<ID3D12Device> &device, UINT byteLength, LPCWSTR name)
{
    D3D12_HEAP_PROPERTIES prop{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
    };
    D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = byteLength,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };
    ComPtr<ID3D12Resource> resource;
    ThrowIfFailed(device->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&resource)));

    return std::shared_ptr<ResourceItem>(
        new ResourceItem(resource, D3D12_RESOURCE_STATE_COPY_DEST, name));
}

std::shared_ptr<ResourceItem> ResourceItem::CreateStaticTexture(const ComPtr<ID3D12Device> &device, UINT width, UINT height, LPCWSTR name)
{
    D3D12_HEAP_PROPERTIES prop{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
    };
    D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = width,
        .Height = height,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };
    ComPtr<ID3D12Resource> resource;
    ThrowIfFailed(device->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&resource)));

    return std::shared_ptr<ResourceItem>(
        new ResourceItem(resource, D3D12_RESOURCE_STATE_COPY_DEST, name));
}

std::shared_ptr<ResourceItem> ResourceItem::CreateStaticCubemap(const ComPtr<ID3D12Device> &device, UINT width, UINT height, LPCWSTR name)
{
    D3D12_HEAP_PROPERTIES prop{
        .Type = D3D12_HEAP_TYPE_DEFAULT,
    };
    D3D12_RESOURCE_DESC desc{
        .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .Alignment = 0,
        .Width = width,
        .Height = height,
        .DepthOrArraySize = 6, // cube !
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc = {1, 0},
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
    };

    auto state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    ComPtr<ID3D12Resource> resource;
    ThrowIfFailed(device->CreateCommittedResource(
        &prop,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        state,
        nullptr,
        IID_PPV_ARGS(&resource)));

    auto item = std::shared_ptr<ResourceItem>(
        new ResourceItem(resource, state, name));
    item->m_state.Upload = UploadStates::Uploaded;
    return item;
}

} // namespace Gpu::dx12
