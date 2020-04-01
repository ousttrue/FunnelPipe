#pragma once
#include "Helper.h"
#include <vector>
#include <memory>

namespace Gpu::dx12
{

struct RenderTargetResources
{
    ComPtr<ID3D12Resource> renderTarget;
    ComPtr<ID3D12Resource> depthStencil;
    void CreateDepthResource(const ComPtr<ID3D12Device> &device);
};

class RenderTargetChain
{
    D3D12_VIEWPORT m_viewport = {};
    D3D12_RECT m_scissorRect = {};

    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
    void CreateHeap(const ComPtr<ID3D12Device> &device);

    std::vector<RenderTargetResources> m_resources;

    bool m_isSwapchain = false;

public:
    void Release()
    {
        m_resources.clear();
    }

private:
    bool Resize(UINT width, UINT height)
    {
        if (m_viewport.Width == width && m_viewport.Height == height)
        {
            return false;
        }

        m_viewport = {
            .Width = (float)width,
            .Height = (float)height,
            .MinDepth = 0,
            .MaxDepth = 1.0f,
        };
        m_scissorRect = {
            .right = (LONG)width,
            .bottom = (LONG)height,
        };
        return true;
    }

public:
    void Initialize(const ComPtr<IDXGISwapChain3> &swapChain,
                    const ComPtr<ID3D12Device> &device,
                    UINT frameCount);

    bool Initialize(UINT width, UINT height,
                    const ComPtr<ID3D12Device> &device,
                    UINT frameCount);

    bool Begin(UINT frameIndex,
               const ComPtr<ID3D12GraphicsCommandList> &commandList, const float *clearColor);
    void End(UINT frameIndex, const ComPtr<ID3D12GraphicsCommandList> &commandList);

    RenderTargetResources *Resource(UINT frameIndex)
    {
        if (frameIndex >= m_resources.size())
        {
            return nullptr;
        }
        return &m_resources[frameIndex];
    }
};

} // namespace Gpu::dx12
