#pragma once
#include "Helper.h"
#include "Heap.h"
#include "ConstantBuffer.h"
#include "CommandList.h"
#include <memory>
#include <array>
#include <unordered_map>
#include <FrameData.h>
#include <DirectXMath.h>

namespace Gpu::dx12
{

/// Shader spec
///
/// * each ConstantBuffer type
/// * manipulation of HeapDescriptor
///
class RootSignature : NonCopyable
{
    ComPtr<ID3D12RootSignature> m_rootSignature;
    std::unique_ptr<Heap> m_heap;
    std::vector<std::pair<uint32_t, uint32_t>> m_viewList;
    struct SRVStatus
    {
        UINT index;
        bool status;
    };
    std::vector<SRVStatus> m_srvStatus;

    std::unordered_map<framedata::FrameMaterialPtr, std::shared_ptr<class Material>> m_materialMap;
    std::unordered_map<framedata::FrameImagePtr, std::shared_ptr<class Texture>> m_textureMap;

public:
    RootSignature();
    bool Initialize(const ComPtr<ID3D12Device> &device);
    // polling shader update
    void Update(const ComPtr<ID3D12Device> &device);
    void Begin(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12GraphicsCommandList> &commandList);
    std::shared_ptr<class Material> GetOrCreate(const ComPtr<ID3D12Device> &device, const framedata::FrameMaterialPtr &material);
    std::shared_ptr<class Texture> GetOrCreate(const ComPtr<ID3D12Device> &device, const framedata::FrameImagePtr &image, class Uploader *uploader);

    // each Frame
    Gpu::dx12::ConstantBuffer m_viewConstantsBuffer;
    // each DrawCall
    Gpu::dx12::ConstantBuffer m_drawConstantsBuffer;

    void SetDrawDescriptorTable(const ComPtr<ID3D12Device> &device,
                                const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT nodeIndex);

    void UpdateSRV(const ComPtr<ID3D12Device> &device,
                   Gpu::dx12::CommandList *commandList,
                   const framedata::FrameData &framedata, class Uploader *uploader);

    bool SetTextureDescriptorTable(const ComPtr<ID3D12Device> &device,
                                   const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT maerialIndex);
};

} // namespace Gpu::dx12
