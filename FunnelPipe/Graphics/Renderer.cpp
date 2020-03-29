#include "Renderer.h"
#include "ImGuiDX12.h"
#include "SceneMapper.h"
#include <FrameData.h>
#include <Gpu.h>

#include <plog/Log.h>
#include <wrl/client.h>

const uint32_t BACKBUFFER_COUNT = 2;

template <class T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

class Impl
{
    std::unique_ptr<Gpu::dx12::SwapChain> m_swapchain;
    std::unique_ptr<Gpu::dx12::RenderTargetChain> m_backbuffer;
    int m_width = 0;
    int m_height = 0;

    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    std::unique_ptr<Gpu::dx12::CommandQueue> m_queue;
    std::unique_ptr<Gpu::dx12::CommandList> m_commandlist;
    std::unique_ptr<Gpu::dx12::SceneMapper> m_sceneMapper;
    std::unique_ptr<Gpu::dx12::RootSignature> m_rootSignature;

    ImGuiDX12 m_imguiDX12;

    // scene
    std::unique_ptr<hierarchy::SceneLight> m_light;

public:
    Impl(int maxModelCount)
        : m_queue(new Gpu::dx12::CommandQueue),
          m_swapchain(new Gpu::dx12::SwapChain),
          m_backbuffer(new Gpu::dx12::RenderTargetChain),
          m_commandlist(new Gpu::dx12::CommandList),
          m_sceneMapper(new Gpu::dx12::SceneMapper),
          m_rootSignature(new Gpu::dx12::RootSignature),
          m_light(new hierarchy::SceneLight)
    {
    }

    void Initialize(HWND hwnd)
    {
        assert(!m_device);

        ComPtr<IDXGIFactory4> factory;
        Gpu::dx12::ThrowIfFailed(CreateDXGIFactory2(Gpu::dx12::GetDxgiFactoryFlags(), IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> hardwareAdapter = Gpu::dx12::GetHardwareAdapter(factory.Get());
        Gpu::dx12::ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)));

        m_queue->Initialize(m_device);
        m_swapchain->Initialize(factory, m_queue->Get(), hwnd, BACKBUFFER_COUNT);
        m_backbuffer->Initialize(m_swapchain->Get(), m_device, BACKBUFFER_COUNT);
        m_sceneMapper->Initialize(m_device);
        m_commandlist->InitializeDirect(m_device);
        m_rootSignature->Initialize(m_device);

        m_imguiDX12.Initialize(m_device.Get(), BACKBUFFER_COUNT);

        //
        // settings
        // https://blog.techlab-xe.net/dx12-debug-id3d12infoqueue/
        //
        ComPtr<ID3D12InfoQueue> infoQueue;
        m_device.As(&infoQueue);

        D3D12_MESSAGE_ID denyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
        };
        D3D12_MESSAGE_SEVERITY severities[] = {
            D3D12_MESSAGE_SEVERITY_INFO};
        D3D12_INFO_QUEUE_FILTER filter{};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;

        infoQueue->PushStorageFilter(&filter);

        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }

    void BeginFrame(HWND hwnd, int width, int height)
    {
        UpdateBackbuffer(hwnd, width, height);
        m_sceneMapper->Update(m_device);
        m_rootSignature->Update(m_device);

        // new frame
        m_commandlist->Reset(nullptr);
    }

    void EndFrame()
    {
        auto commandList = m_commandlist->Get();
        auto frameIndex = m_swapchain->CurrentFrameIndex();

        // barrier
        float clear[4] = {0.2f, 0.4f, 0.3f, 1.0f};
        m_backbuffer->Begin(frameIndex, commandList, clear);

        ImGui::Render();
        m_imguiDX12.RenderDrawData(commandList.Get(), ImGui::GetDrawData());

        m_backbuffer->End(frameIndex, commandList);

        // execute
        auto callbacks = m_commandlist->CloseAndGetCallbacks();
        m_queue->Execute(commandList);
        m_swapchain->Present();
        m_queue->SyncFence(callbacks);
    }

    size_t ViewTextureID(size_t id)
    {
        // view texture for current frame
        auto viewRenderTarget = m_sceneMapper->GetOrCreateRenderTarget(id);
        auto resource = viewRenderTarget->Resource(m_swapchain->CurrentFrameIndex());
        size_t texture = resource ? m_imguiDX12.GetOrCreateTexture(m_device.Get(), resource->renderTarget.Get()) : -1;
        return texture;
    }

    void View(const framedata::FrameData &drawlist)
    {
        auto viewRenderTarget = m_sceneMapper->GetOrCreateRenderTarget((size_t)&drawlist);
        UpdateView(viewRenderTarget, drawlist);
        UpdateMeshes(drawlist);
        DrawView(m_commandlist->Get(), m_swapchain->CurrentFrameIndex(), viewRenderTarget,
                 drawlist.ViewClearColor.data(), drawlist);
    }

private:
    void UpdateBackbuffer(HWND hwnd, int width, int height)
    {
        if (m_width != width || m_height != height)
        {
            // recreate swapchain
            m_queue->SyncFence();
            m_backbuffer->Release(); // require before resize
            m_swapchain->Resize(m_queue->Get(),
                                hwnd, BACKBUFFER_COUNT, width, height);
            m_backbuffer->Initialize(m_swapchain->Get(), m_device, BACKBUFFER_COUNT);

            m_width = width;
            m_height = height;
        }
    }

    void UpdateMeshes(const framedata::FrameData &drawlist)
    {
        for (size_t i = 0; i < drawlist.Meshlist.size(); ++i)
        {
            auto &item = drawlist.Meshlist[i];
            auto mesh = item.Mesh;
            auto drawable = m_sceneMapper->GetOrCreate(m_device, item.Mesh);
            if (drawable)
            {
                if (item.Skin.Ptr)
                {
                    drawable->VertexBuffer()->MapCopyUnmap(item.Skin.Ptr, item.Skin.Size, item.Skin.Stride);
                }
                else if (item.Vertices.Ptr)
                {
                    drawable->VertexBuffer()->MapCopyUnmap(item.Vertices.Ptr, item.Vertices.Size, item.Vertices.Stride);
                }

                if (item.Indices.Ptr)
                {
                    drawable->IndexBuffer()->MapCopyUnmap(item.Indices.Ptr, item.Indices.Size, item.Indices.Stride);
                }
            }
        }

        // CB
        m_rootSignature->m_drawConstantsBuffer.Assign((const std::pair<UINT, UINT> *)drawlist.CBRanges.data(),
                                                      (uint32_t)drawlist.CBRanges.size());
        m_rootSignature->m_drawConstantsBuffer.CopyToGpu(drawlist.CB.data(), drawlist.CB.size());
    }

    void UpdateView(const std::shared_ptr<Gpu::dx12::RenderTargetChain> &viewRenderTarget,
                    const framedata::FrameData &drawlist)
    {
        m_rootSignature->m_viewConstantsBuffer.CopyToGpu(drawlist.ViewConstantBuffer);

        if (viewRenderTarget->Resize(drawlist.ViewWidth(), drawlist.ViewHeight()))
        {
            // clear all
            for (UINT i = 0; i < BACKBUFFER_COUNT; ++i)
            {
                auto resource = viewRenderTarget->Resource(i);
                if (resource)
                {
                    m_imguiDX12.Remove(resource->renderTarget.Get());
                }
            }
            viewRenderTarget->Initialize(drawlist.ViewWidth(), drawlist.ViewHeight(), m_device, BACKBUFFER_COUNT);
        }
    }

    void DrawView(const ComPtr<ID3D12GraphicsCommandList> &commandList, int frameIndex,
                  const std::shared_ptr<Gpu::dx12::RenderTargetChain> &viewRenderTarget,
                  const float *clearColor,
                  const framedata::FrameData &drawlist)
    {
        // begin, clear
        if (viewRenderTarget->Begin(frameIndex, commandList, clearColor))
        {
            // setup root signature
            // all shader use same root signature
            m_rootSignature->Begin(m_device, commandList);

            for (size_t i = 0; i < drawlist.Drawlist.size(); ++i)
            {
                DrawMesh(commandList, (UINT)i, drawlist.Drawlist[i]);
            }

            // finish rendering
            viewRenderTarget->End(frameIndex, commandList);
        }
    }

    void DrawMesh(const ComPtr<ID3D12GraphicsCommandList> &commandList, UINT i,
                  const framedata::FrameData::DrawItem &info)
    {
        auto drawable = m_sceneMapper->GetOrCreate(m_device, info.Mesh);
        if (!drawable)
        {
            return;
        }
        auto [isDrawable, callback] = drawable->IsDrawable(commandList);
        if (callback)
        {
            m_commandlist->AddOnCompleted(callback);
        }
        if (!isDrawable)
        {
            return;
        }

        m_rootSignature->SetDrawDescriptorTable(m_device, commandList, i);

        auto &submesh = info.Submesh;
        auto material = m_rootSignature->GetOrCreate(m_device, submesh.material);

        // texture setup
        if (submesh.material->colorImage)
        {
            auto [texture, textureSlot] = m_rootSignature->GetOrCreate(m_device, submesh.material->colorImage,
                                                                       m_sceneMapper->GetUploader());
            if (texture)
            {
                auto [isDrawable, callback] = texture->IsDrawable(commandList);
                if (callback)
                {
                    m_commandlist->AddOnCompleted(callback);
                }
                if (isDrawable)
                {
                    m_rootSignature->SetTextureDescriptorTable(m_device, commandList, textureSlot);
                }
            }
        }

        if (material->Set(commandList))
        {
            commandList->DrawIndexedInstanced(submesh.drawCount, 1, submesh.drawOffset, 0, 0);
        }
    }
};

Renderer::Renderer(int maxModelCount)
    : m_impl(new Impl(maxModelCount))
{
}

Renderer::~Renderer()
{
    delete m_impl;
}

void Renderer::Initialize(void *hwnd)
{
    m_impl->Initialize((HWND)hwnd);
}

void Renderer::BeginFrame(void *hwnd, int width, int height)
{
    m_impl->BeginFrame((HWND)hwnd, width, height);
}

void Renderer::EndFrame()
{
    m_impl->EndFrame();
}

size_t Renderer::ViewTextureID(size_t view)
{
    return m_impl->ViewTextureID(view);
}

void Renderer::View(const framedata::FrameData &drawlist)
{
    m_impl->View(drawlist);
}
