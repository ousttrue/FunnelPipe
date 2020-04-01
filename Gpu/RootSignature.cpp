#include "RootSignature.h"
#include "Shader.h"
#include "Material.h"
#include "ResourceItem.h"
#include "Texture.h"
#include "Uploader.h"
#include <d3dcompiler.h>
#include <algorithm>

const int FRAME_SLOTS = 1;
const int DRAW_SLOTS = 1024;
const int TEXTURE_SLOTS = 1024;

enum class RootDescriptorSlots
{
    CBV_0,
    CBV_1,
    CBV_2,
    SRV,
    Sampler,
};

namespace Gpu::dx12
{

RootSignature::RootSignature()
    : m_CBV_SRV_UAV_Heap(new Heap), m_Sampler_Heap(new Heap)
{
}

bool RootSignature::Initialize(const ComPtr<ID3D12Device> &device)
{
    // Create a root signature consisting of a descriptor table with a single CBV.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {
        // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
        .HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1,
    };
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        throw;
    }

    D3D12_DESCRIPTOR_RANGE1 ranges[] = {
        {
            // b0
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .NumDescriptors = 1,
            .BaseShaderRegister = 0,
            .RegisterSpace = 0,
            .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
            // . OffsetInDescriptorsFromTableStart,
        },
        {
            // b1 for VS
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .NumDescriptors = 1,
            .BaseShaderRegister = 1,
            .RegisterSpace = 0,
            .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
            // . OffsetInDescriptorsFromTableStart,
        },
        {
            // b1 for PS
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
            .NumDescriptors = 1,
            .BaseShaderRegister = 1,
            .RegisterSpace = 0,
            .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
            // . OffsetInDescriptorsFromTableStart,
        },
        {
            // t0-t7
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
            .NumDescriptors = 8,
            .BaseShaderRegister = 0,
            .RegisterSpace = 0,
            // .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
        },
        {
            // s0-s7
            .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
            .NumDescriptors = 8,
            .BaseShaderRegister = 0,
            .RegisterSpace = 0,
        },
    };

    D3D12_ROOT_PARAMETER1 rootParameters[] = {
        // b0
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable = {
                .NumDescriptorRanges = 1,
                .pDescriptorRanges = &ranges[0],
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
        },
        // b1 for VS
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable = {
                .NumDescriptorRanges = 1,
                .pDescriptorRanges = &ranges[1],
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
        },
        // b1 for PS
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable = {
                .NumDescriptorRanges = 1,
                .pDescriptorRanges = &ranges[2],
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
        },
        // t0-t7
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable = {
                .NumDescriptorRanges = 1,
                .pDescriptorRanges = &ranges[3],
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
        },
        // s0-s7
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
            .DescriptorTable = {
                .NumDescriptorRanges = 1,
                .pDescriptorRanges = &ranges[4],
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL,
        },
    };

    // Allow input layout and deny unecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT //
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS     //
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS   //
        | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS //
        // | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS      //
        ;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc{
        .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
        .Desc_1_1{
            .NumParameters = _countof(rootParameters),
            .pParameters = rootParameters,
            .Flags = rootSignatureFlags,
        },
    };

    D3D12_STATIC_SAMPLER_DESC sampler = {
        .Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
        .MaxLOD = D3D12_FLOAT32_MAX,
        .ShaderRegister = 12,
        .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL,
    };
    rootSignatureDesc.Desc_1_1.pStaticSamplers = &sampler;
    rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
    ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    //
    // buffers
    //
    m_viewConstantsBuffer.Initialize(device, 1024);
    m_viewConstantsBuffer.AllRange();

    m_drawConstantsBuffer.Initialize(device, 1024 * DRAW_SLOTS);
    m_CBV_SRV_UAV_Heap->Initialize(device,
                                   D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                                   FRAME_SLOTS + DRAW_SLOTS + TEXTURE_SLOTS);

    {
        auto [offset, size] = m_viewConstantsBuffer.Range(0);
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
            .BufferLocation = m_viewConstantsBuffer.Resource()->GetGPUVirtualAddress() + offset,
            .SizeInBytes = size,
        };
        device->CreateConstantBufferView(&cbvDesc, m_CBV_SRV_UAV_Heap->CpuHandle(0));
    }

    // sampler
    m_Sampler_Heap->Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, TEXTURE_SLOTS);
    auto handleSampler = m_Sampler_Heap->Get()->GetCPUDescriptorHandleForHeapStart();
    for (int i = 0; i < 8; ++i)
    {
        D3D12_SAMPLER_DESC descSampler{
            .Filter = D3D12_FILTER_MIN_MAG_MIP_POINT,
            .AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
            .MipLODBias = 0.0F,
            .MaxAnisotropy = 0,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .MinLOD = -FLT_MAX,
            .MaxLOD = FLT_MAX,
        };

        device->CreateSampler(&descSampler, handleSampler);
        handleSampler.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    }

    return true;
}

void RootSignature::Update(const ComPtr<ID3D12Device> &device)
{
    for (auto kv : m_materialMap)
    {
        kv.second->Initialize(device, kv.first);
    }
}

void RootSignature::Begin(const ComPtr<ID3D12Device> &device, const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    ID3D12DescriptorHeap *ppHeaps[] = {
        m_CBV_SRV_UAV_Heap->Get(),
        m_Sampler_Heap->Get(),
    };
    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
    commandList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootDescriptorSlots::CBV_0),
        m_CBV_SRV_UAV_Heap->GpuHandle(0));
}

std::shared_ptr<Material> RootSignature::GetOrCreate(const ComPtr<ID3D12Device> &device, const std::shared_ptr<framedata::FrameMaterial> &sceneMaterial)
{
    auto found = m_materialMap.find(sceneMaterial);
    if (found != m_materialMap.end())
    {
        return found->second;
    }

    auto gpuMaterial = std::make_shared<Material>();
    if (!gpuMaterial->Initialize(device, m_rootSignature, sceneMaterial))
    {
        throw;
    }

    m_materialMap.insert(std::make_pair(sceneMaterial, gpuMaterial));
    return gpuMaterial;
}

std::shared_ptr<class Texture> RootSignature::GetOrCreate(
    const ComPtr<ID3D12Device> &device,
    const framedata::FrameTexturePtr &texture,
    Uploader *uploader)
{
    auto found = m_textureMap.find(texture);
    if (found != m_textureMap.end())
    {
        return found->second;
    }

    // create texture
    auto gpuTexture = std::make_shared<Texture>();

    if (texture->Images.size() == 6)
    {
        // cube
        auto image = texture->Images.front();
        auto resource = ResourceItem::CreateStaticCubemap(device, image->width, image->height, Utf8ToUnicode(image->name).c_str());
        gpuTexture->ImageBuffer(resource);
        // TODO
        // uploader->EnqueueUpload(resource, image->buffer.data(), (UINT)image->buffer.size(), image->width * 4);
    }
    else if (texture->Images.size() == 1)
    {
        // 2d
        auto image = texture->Images.front();
        auto resource = ResourceItem::CreateStaticTexture(device, image->width, image->height, Utf8ToUnicode(image->name).c_str());
        gpuTexture->ImageBuffer(resource);
        uploader->EnqueueUpload(resource, image->buffer.data(), (UINT)image->buffer.size(), image->width * 4);
    }
    else
    {
        throw;
    }

    m_textureMap.insert(std::make_pair(texture, gpuTexture));

    return gpuTexture;
}

void RootSignature::SetDrawDescriptorTable(const ComPtr<ID3D12Device> &device,
                                           const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT nodeIndex)
{
    std::pair<uint32_t, uint32_t> view = {};
    if (nodeIndex < m_viewList.size())
    {
        view = m_viewList[nodeIndex];
    }
    else
    {
        m_viewList.resize(nodeIndex + 1);
    }

    auto [offset, size] = m_drawConstantsBuffer.Range(nodeIndex);
    if (view.first != offset || view.second != size)
    {
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
            .BufferLocation = m_drawConstantsBuffer.Resource()->GetGPUVirtualAddress() + offset,
            .SizeInBytes = size,
        };
        device->CreateConstantBufferView(&cbvDesc, m_CBV_SRV_UAV_Heap->CpuHandle(1 + nodeIndex));
        m_viewList[nodeIndex] = {offset, size};
    }

    commandList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootDescriptorSlots::CBV_1),
        m_CBV_SRV_UAV_Heap->GpuHandle(1 + nodeIndex));
}

void RootSignature::UpdateSRV(const ComPtr<ID3D12Device> &device,
                              Gpu::dx12::CommandList *commandList,
                              const framedata::FrameData &framedata, class Uploader *uploader)
{
    std::vector<std::shared_ptr<Texture>> m_textures;
    m_textures.clear();
    m_srvStatus.clear();

    for (auto &texture : framedata.Textures)
    {
        auto gpuTexture = GetOrCreate(device, texture, uploader);
        if (gpuTexture)
        {
            auto [isDrawable, callback] = gpuTexture->IsDrawable(commandList->Get());
            if (callback)
            {
                commandList->AddOnCompleted(callback);
            }
            if (!isDrawable)
            {
                gpuTexture = nullptr;
            }
        }
        m_textures.push_back(gpuTexture);
    }

    UINT index = FRAME_SLOTS + DRAW_SLOTS;
    for (auto &srv : framedata.SRVViews)
    {
        bool status = true;
        m_srvStatus.push_back({
            .index = index,
        });
        for (int i = 0; i < 8; ++i, ++index)
        {
            // create view
            auto gpuTexture = m_textures[srv.list[i]];
            if (gpuTexture)
            {
                if (gpuTexture->IsCubeMap)
                {
                    assert(gpuTexture->Resource()->GetDesc().DepthOrArraySize == 6);
                    D3D12_SHADER_RESOURCE_VIEW_DESC desc{
                        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE,
                        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                        .TextureCube = {
                            .MostDetailedMip = 0,
                            .MipLevels = 1,
                            .ResourceMinLODClamp = 0.0f,
                        },
                    };
                    device->CreateShaderResourceView(gpuTexture->Resource().Get(), &desc, m_CBV_SRV_UAV_Heap->CpuHandle(index));
                }
                else
                {
                    D3D12_SHADER_RESOURCE_VIEW_DESC desc{
                        .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                        .ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
                        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                        .Texture2D = {
                            .MostDetailedMip = 0,
                            .MipLevels = 1,
                        },
                    };
                    device->CreateShaderResourceView(gpuTexture->Resource().Get(), &desc, m_CBV_SRV_UAV_Heap->CpuHandle(index));
                }
            }
            else
            {
                // not ready
                status = false;
            }

            //
        }
        m_srvStatus.back().status = status;
    }
}

bool RootSignature::SetTextureDescriptorTable(const ComPtr<ID3D12Device> &device,
                                              const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT materialIndex)
{
    auto &status = m_srvStatus[materialIndex];
    if (!status.status)
    {
        return false;
    }
    commandList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootDescriptorSlots::SRV),
        m_CBV_SRV_UAV_Heap->GpuHandle(status.index));
    commandList->SetGraphicsRootDescriptorTable(
        static_cast<UINT>(RootDescriptorSlots::Sampler),
        m_Sampler_Heap->GpuHandle(0));
    return true;
}

} // namespace Gpu::dx12
