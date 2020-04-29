#include "../Renderer.h"
#include "../SceneMapper.h"
#include "ImGuiDX12.h"
#include <FrameData.h>
#include <Gpu.h>

#include <plog/Log.h>
#include <wrl/client.h>

const uint32_t BACKBUFFER_COUNT = 2;

template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

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

    int m_viewWidth = 0;
    int m_viewHeight = 0;

public:
    Impl(int maxModelCount)
        : m_queue(new Gpu::dx12::CommandQueue),
          m_swapchain(new Gpu::dx12::SwapChain),
          m_backbuffer(new Gpu::dx12::RenderTargetChain),
          m_commandlist(new Gpu::dx12::CommandList),
          m_sceneMapper(new Gpu::dx12::SceneMapper),
          m_rootSignature(new Gpu::dx12::RootSignature)
    {
    }

    void Initialize(HWND hwnd)
    {
        assert(!m_device);

        ComPtr<IDXGIFactory4> factory;
        Gpu::dx12::ThrowIfFailed(CreateDXGIFactory2(
            Gpu::dx12::GetDxgiFactoryFlags(), IID_PPV_ARGS(&factory)));

        ComPtr<IDXGIAdapter1> hardwareAdapter =
            Gpu::dx12::GetHardwareAdapter(factory.Get());
        Gpu::dx12::ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(),
                                                   D3D_FEATURE_LEVEL_11_0,
                                                   IID_PPV_ARGS(&m_device)));

        m_queue->Initialize(m_device);
        m_swapchain->Initialize(factory, m_queue->Get(), hwnd,
                                BACKBUFFER_COUNT);
        m_backbuffer->Initialize(m_swapchain->Get(), m_device,
                                 BACKBUFFER_COUNT);
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
        D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
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

    ComPtr<ID3D12Resource> ViewTexture(size_t id)
    {
        auto viewRenderTarget = m_sceneMapper->GetOrCreateRenderTarget(id);
        auto resource =
            viewRenderTarget->Resource(m_swapchain->CurrentFrameIndex());
        if (!resource)
        {
            return nullptr;
        }
        return resource->renderTarget;
    }

    void View(const framedata::FrameData &framedata)
    {
        auto viewRenderTarget =
            m_sceneMapper->GetOrCreateRenderTarget((size_t)&framedata);
        int width = framedata.ViewWidth();
        int height = framedata.ViewHeight();
        int frameIndex = m_swapchain->CurrentFrameIndex();
        // if (viewRenderTarget->SizeChanged(width, height))
        // {
        //     // clear
        //     // m_queue->SyncFence();
        //     if (viewRenderTarget->Release())
        //     {
        //         return;
        //     }
        // }
        bool changed = width != m_viewWidth || height != m_viewHeight;
        if (changed)
        {
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                m_viewWidth = width;
                m_viewHeight = height;
                viewRenderTarget->Release();
            }
        }
        if (m_width == 0 || m_height == 0)
        {
            return;
        }

        if (!viewRenderTarget->Resource(frameIndex))
        {
            viewRenderTarget->Initialize(width, height, m_device,
                                         BACKBUFFER_COUNT);
        }

        m_rootSignature->m_viewConstantsBuffer.CopyToGpu(
            framedata.ViewConstantBuffer);
        UpdateMeshes(framedata);
        m_rootSignature->UpdateSRV(m_device, m_commandlist.get(), framedata,
                                   m_sceneMapper->GetUploader());
        DrawView(m_commandlist->Get(), frameIndex, viewRenderTarget,
                 framedata.ViewClearColor.data(), framedata);
    }

    Microsoft::WRL::ComPtr<ID3D12Resource>
    GetTexture(const framedata::FrameTexturePtr &texture)
    {
        auto resource = m_rootSignature->GetOrCreate(
            m_device, texture, m_sceneMapper->GetUploader());
        return resource->Resource();
    }

private:
    void UpdateBackbuffer(HWND hwnd, int width, int height)
    {
        if (m_width != width || m_height != height)
        {
            // recreate swapchain
            m_queue->SyncFence();
            m_backbuffer->Release(); // require before resize
            m_swapchain->Resize(m_queue->Get(), hwnd, BACKBUFFER_COUNT, width,
                                height);
            m_backbuffer->Initialize(m_swapchain->Get(), m_device,
                                     BACKBUFFER_COUNT);

            m_width = width;
            m_height = height;
        }
    }

    void UpdateMeshes(const framedata::FrameData &framedata)
    {
        for (size_t i = 0; i < framedata.Meshlist.size(); ++i)
        {
            auto &item = framedata.Meshlist[i];
            auto mesh = item.Mesh;
            auto drawable = m_sceneMapper->GetOrCreate(m_device, item.Mesh);
            if (drawable)
            {
                if (item.Skin.Ptr)
                {
                    drawable->VertexBuffer()->MapCopyUnmap(
                        item.Skin.Ptr, item.Skin.Size, item.Skin.Stride);
                }
                else if (item.Vertices.Ptr)
                {
                    drawable->VertexBuffer()->MapCopyUnmap(
                        item.Vertices.Ptr, item.Vertices.Size,
                        item.Vertices.Stride);
                }

                if (item.Indices.Ptr)
                {
                    drawable->IndexBuffer()->MapCopyUnmap(item.Indices.Ptr,
                                                          item.Indices.Size,
                                                          item.Indices.Stride);
                }
            }
        }

        // CB
        m_rootSignature->m_drawConstantsBuffer.Assign(
            (const std::pair<UINT, UINT> *)framedata.CBRanges.data(),
            (uint32_t)framedata.CBRanges.size());
        m_rootSignature->m_drawConstantsBuffer.CopyToGpu(framedata.CB.data(),
                                                         framedata.CB.size());
    }

    void DrawView(
        const ComPtr<ID3D12GraphicsCommandList> &commandList, int frameIndex,
        const std::shared_ptr<Gpu::dx12::RenderTargetChain> &viewRenderTarget,
        const float *clearColor, const framedata::FrameData &framedata)
    {
        // begin, clear
        if (viewRenderTarget->Begin(frameIndex, commandList, clearColor))
        {
            // setup root signature
            // all shader use same root signature
            m_rootSignature->Begin(m_device, commandList);

            for (size_t i = 0; i < framedata.Drawlist.size(); ++i)
            {
                DrawMesh(commandList, (UINT)i, framedata.Drawlist[i]);
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

        auto cbIndex = i * 2;
        m_rootSignature->SetDrawDescriptorTable(
            m_device, commandList, D3D12_SHVER_VERTEX_SHADER, cbIndex);
        m_rootSignature->SetDrawDescriptorTable(
            m_device, commandList, D3D12_SHVER_PIXEL_SHADER, cbIndex + 1);

        auto &submesh = info.Submesh;
        auto material =
            m_rootSignature->GetOrCreate(m_device, submesh.material);

        m_rootSignature->SetTextureDescriptorTable(m_device, commandList,
                                                   info.MaterialIndex);
        if (material->SetPipeline(commandList))
        {
            commandList->DrawIndexedInstanced(submesh.drawCount, 1,
                                              submesh.drawOffset, 0, 0);
        }
    }
};

Renderer::Renderer(int maxModelCount) : m_impl(new Impl(maxModelCount)) {}

Renderer::~Renderer() { delete m_impl; }

void Renderer::Initialize(void *hwnd) { m_impl->Initialize((HWND)hwnd); }

void Renderer::BeginFrame(void *hwnd, int width, int height)
{
    m_impl->BeginFrame((HWND)hwnd, width, height);
}

void Renderer::EndFrame() { m_impl->EndFrame(); }

void *Renderer::ViewTexture(size_t view)
{
    auto p = m_impl->ViewTexture(view).Get();
    if (p)
    {
        p->AddRef();
    }
    return p;
}

void Renderer::ReleaseViewTexture(void *viewTexture)
{
    auto p = (ID3D12Resource *)viewTexture;
    if (p)
    {
        p->Release();
    }
}

void Renderer::View(const framedata::FrameData &framedata)
{
    m_impl->View(framedata);
}

void *Renderer::GetTexture(const framedata::FrameTexturePtr &texture)
{
    auto p = m_impl->GetTexture(texture).Get();
    if (p)
    {
        p->AddRef();
    }
    return p;
}
