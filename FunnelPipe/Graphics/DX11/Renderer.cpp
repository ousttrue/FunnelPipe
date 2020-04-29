#include "../Renderer.h"
#include <wrl/client.h>
#include <Gpu.h>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>

const uint32_t BACKBUFFER_COUNT = 2;

template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

static ComPtr<IDXGIAdapter1> GetHardwareAdapter()
{
    UINT flags = 0;
#ifdef _DEBUG
    flags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    ComPtr<IDXGIFactory4> factory;
    Gpu::ThrowIfFailed(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)));

    // enumerate
    ComPtr<IDXGIAdapter1> adapter;
    for (UINT adapterIndex = 0;
         DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter);
         ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            // If you want a software adapter, pass in "/warp" on the command
            // line.
            continue;
        }
        return adapter;
    }

    return nullptr;
}

static ComPtr<IDXGIFactory2>
FactoryFromDevice(const ComPtr<ID3D11Device> &device)
{
    ComPtr<IDXGIDevice2> pDXGIDevice;
    Gpu::ThrowIfFailed(device.As(&pDXGIDevice));

    ComPtr<IDXGIAdapter> pDXGIAdapter;
    Gpu::ThrowIfFailed(pDXGIDevice->GetParent(IID_PPV_ARGS(&pDXGIAdapter)));

    ComPtr<IDXGIFactory2> pIDXGIFactory;
    Gpu::ThrowIfFailed(pDXGIAdapter->GetParent(IID_PPV_ARGS(&pIDXGIFactory)));

    return pIDXGIFactory;
}

class Impl
{
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain1> m_swapchain;
    DXGI_SWAP_CHAIN_DESC1 m_swapchainDesc = {};

public:
    Impl() {}

    ~Impl()
    {
        ImGui_ImplDX11_Shutdown();
    }

    void Initialize(HWND hwnd)
    {
        auto adapter = GetHardwareAdapter();
        if (!adapter)
        {
            throw std::runtime_error("no hardware adapter");
        }

        UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };
        D3D_FEATURE_LEVEL featureLevel;
        Gpu::ThrowIfFailed(D3D11CreateDevice(
            adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags,
            featureLevels, _countof(featureLevels), D3D11_SDK_VERSION,
            &m_device, &featureLevel, &m_context));

        // IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        ImGui_ImplDX11_Init(m_device.Get(), m_context.Get());
        ImGui_ImplDX11_NewFrame();
    }

    void BeginFrame(HWND hwnd, int width, int height)
    {
        if (m_swapchain)
        {
            if (m_swapchainDesc.Width != width ||
                m_swapchainDesc.Height != height)
            {
                // clear all backbuffer

                // resize
                Gpu::ThrowIfFailed(m_swapchain->ResizeBuffers(
                    BACKBUFFER_COUNT, width, height, m_swapchainDesc.Format,
                    m_swapchainDesc.Flags));
                m_swapchain->GetDesc1(&m_swapchainDesc);
            }
        }
        else
        {
            DXGI_SWAP_CHAIN_DESC1 desc = {
                .Width = (UINT)width,
                .Height = (UINT)height,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc =
                    {
                        .Count = 1,
                    },
                .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
                .BufferCount = BACKBUFFER_COUNT,
                .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            };
            Gpu::ThrowIfFailed(
                FactoryFromDevice(m_device)->CreateSwapChainForHwnd(
                    m_device.Get(), hwnd, &desc, nullptr, nullptr,
                    &m_swapchain));
            m_swapchain->GetDesc1(&m_swapchainDesc);
        }
    }

    void EndFrame()
    {
        // draw
        ImGui::Render();

        // get backbuffer and set rtv
        ComPtr<ID3D11Texture2D> backbuffer;
        Gpu::ThrowIfFailed(
            m_swapchain->GetBuffer(0, IID_PPV_ARGS(&backbuffer)));
        ComPtr<ID3D11RenderTargetView> rtv;
        Gpu::ThrowIfFailed(
            m_device->CreateRenderTargetView(backbuffer.Get(), nullptr, &rtv));

        // clear
        float clearColor[4] = {
            0.6f,
            0.2f,
            0.2f,
            1.0f,
        };
        m_context->ClearRenderTargetView(rtv.Get(), clearColor);

        ID3D11RenderTargetView *rtvs[] = {
            rtv.Get(),
        };
        m_context->OMSetRenderTargets(_countof(rtvs), rtvs, nullptr);

        // draw
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        m_swapchain->Present(1, DXGI_SWAP_EFFECT_DISCARD);
        
        // cleanup
        m_context->OMSetRenderTargets(0, nullptr, nullptr);
    }
};

Renderer::Renderer(int maxModelCount) : m_impl(new Impl()) {}

Renderer::~Renderer() { delete m_impl; }

void Renderer::Initialize(void *hwnd) { m_impl->Initialize((HWND)hwnd); }

void Renderer::BeginFrame(void *hwnd, int width, int height)
{
    m_impl->BeginFrame((HWND)hwnd, width, height);
}

void Renderer::EndFrame() { m_impl->EndFrame(); }

void *Renderer::ViewTexture(size_t view)
{
    return nullptr;
    // auto p = m_impl->ViewTexture(view).Get();
    // if (p)
    // {
    //     p->AddRef();
    // }
    // return p;
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
    // m_impl->View(framedata);
}

void *Renderer::GetTexture(const framedata::FrameTexturePtr &texture)
{
    return nullptr;
    // auto p = m_impl->GetTexture(texture).Get();
    // if (p)
    // {
    //     p->AddRef();
    // }
    // return p;
}
