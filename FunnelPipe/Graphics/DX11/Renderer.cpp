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

class Material
{
public:
    bool Initialize(const ComPtr<ID3D11Device> &device,
                    const framedata::FrameMaterialPtr &material)
    {
        // create shader
        return true;
    }

    void SetPipeline(const ComPtr<ID3D11DeviceContext> &context)
    {
        // setup state
    }
};

class Mesh
{
    ComPtr<ID3D11Buffer> m_vertices;
    ComPtr<ID3D11Buffer> m_indices;

public:
    void VertexBuffer(const ComPtr<ID3D11Buffer> &buffer)
    {
        m_vertices = buffer;
    }
    void IndexBuffer(const ComPtr<ID3D11Buffer> &buffer) { m_indices = buffer; }
};

class Impl
{
    ComPtr<ID3D11Device> m_device;
    ComPtr<ID3D11DeviceContext> m_context;
    ComPtr<IDXGISwapChain1> m_swapchain;
    DXGI_SWAP_CHAIN_DESC1 m_swapchainDesc = {};

    ComPtr<ID3D11Texture2D> m_viewTexture;
    ComPtr<ID3D11ShaderResourceView> m_viewSrv;
    ComPtr<ID3D11Texture2D> m_viewDepth;
    ComPtr<ID3D11Buffer> m_viewConstantBuffer;

public:
    Impl() {}

    ~Impl() { ImGui_ImplDX11_Shutdown(); }

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

    ComPtr<ID3D11ShaderResourceView> ViewTexture() const { return m_viewSrv; }

    void View(const framedata::FrameData &framedata)
    {
        if (m_viewTexture)
        {
            D3D11_TEXTURE2D_DESC desc;
            m_viewTexture->GetDesc(&desc);
            if (framedata.ViewWidth() != desc.Width ||
                framedata.ViewHeight() != desc.Height)
            {
                m_viewTexture.Reset();
            }
        }
        if (!m_viewTexture)
        {
            D3D11_TEXTURE2D_DESC desc{
                .Width = framedata.ViewWidth(),
                .Height = framedata.ViewHeight(),
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc =
                    {
                        1,
                        0,
                    },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags =
                    D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
                .CPUAccessFlags = 0,
                .MiscFlags = 0,
            };
            Gpu::ThrowIfFailed(
                m_device->CreateTexture2D(&desc, nullptr, &m_viewTexture));
            Gpu::ThrowIfFailed(m_device->CreateShaderResourceView(
                m_viewTexture.Get(), nullptr, &m_viewSrv));

            desc.Format = DXGI_FORMAT_D32_FLOAT;
            desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
            Gpu::ThrowIfFailed(
                m_device->CreateTexture2D(&desc, nullptr, &m_viewDepth));
        }

        // update buffers
        UpdateViewConstantBuffer(framedata.ViewConstantBuffer);

        //
        // render target
        //

        // device
        ComPtr<ID3D11RenderTargetView> rtv;
        Gpu::ThrowIfFailed(m_device->CreateRenderTargetView(m_viewTexture.Get(),
                                                            nullptr, &rtv));
        ComPtr<ID3D11DepthStencilView> dsv;
        Gpu::ThrowIfFailed(
            m_device->CreateDepthStencilView(m_viewDepth.Get(), nullptr, &dsv));

        // context
        m_context->ClearRenderTargetView(rtv.Get(),
                                         framedata.ViewClearColor.data());
        m_context->ClearDepthStencilView(dsv.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
        ID3D11RenderTargetView *rtvs[] = {
            rtv.Get(),
        };
        m_context->OMSetRenderTargets(_countof(rtvs), rtvs, dsv.Get());

        //
        // draw
        //
        for (size_t i = 0; i < framedata.Drawlist.size(); ++i)
        {
            DrawMesh((UINT)i, framedata.Drawlist[i]);
        }
    }

private:
    template <typename T> void UpdateViewConstantBuffer(const T &buffer)
    {
        if (!m_viewConstantBuffer)
        {
            D3D11_BUFFER_DESC desc = {
                desc.ByteWidth = sizeof(T),
                desc.Usage = D3D11_USAGE_DEFAULT,
                desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            };
            Gpu::ThrowIfFailed(
                m_device->CreateBuffer(&desc, nullptr, &m_viewConstantBuffer));
        }
        m_context->UpdateSubresource(m_viewConstantBuffer.Get(), 0, nullptr,
                                     &buffer, 0, 0);
    }

    void DrawMesh(UINT i, const framedata::FrameData::DrawItem &info)
    {
        auto drawable = GetOrCreateMesh(info.Mesh);
        if (!drawable)
        {
            return;
        }

        auto &submesh = info.Submesh;
        auto material = GetOrCreateMaterial(submesh.material);

        ID3D11ShaderResourceView *srvs[] = {
            GetOrCreateTexture(submesh.material->ColorTexture).Get(),
        };
        m_context->PSSetShaderResources(0, _countof(srvs), srvs);

        material->SetPipeline(m_context);
        m_context->DrawIndexedInstanced(submesh.drawCount, 1,
                                        submesh.drawOffset, 0, 0);
    }

    std::unordered_map<framedata::FrameMaterialPtr,
                       std::shared_ptr<class Material>>
        m_materialMap;
    std::shared_ptr<Material> GetOrCreateMaterial(
        const std::shared_ptr<framedata::FrameMaterial> &sceneMaterial)
    {
        auto found = m_materialMap.find(sceneMaterial);
        if (found != m_materialMap.end())
        {
            return found->second;
        }

        auto gpuMaterial = std::make_shared<Material>();
        if (!gpuMaterial->Initialize(m_device, sceneMaterial))
        {
            throw;
        }

        m_materialMap.insert(std::make_pair(sceneMaterial, gpuMaterial));
        return gpuMaterial;
    }

    std::unordered_map<framedata::FrameMeshPtr, std::shared_ptr<class Mesh>>
        m_meshMap;
    std::shared_ptr<Mesh>
    GetOrCreateMesh(const std::shared_ptr<framedata::FrameMesh> &sceneMesh)
    {
        auto found = m_meshMap.find(sceneMesh);
        if (found != m_meshMap.end())
        {
            return found->second;
        }

        auto gpuMesh = std::make_shared<Mesh>();

        // vertices
        {
            auto vertices = sceneMesh->vertices;
            ComPtr<ID3D11Buffer> resource;
            if (vertices->isDynamic)
            {
                D3D11_BUFFER_DESC desc{
                    .ByteWidth = static_cast<UINT>(vertices->buffer.size()),
                    .Usage = D3D11_USAGE_DYNAMIC,
                    .BindFlags = D3D11_BIND_VERTEX_BUFFER,
                    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                    .MiscFlags = 0,
                    .StructureByteStride = 0,
                };
                Gpu::ThrowIfFailed(
                    m_device->CreateBuffer(&desc, nullptr, &resource));
            }
            else
            {
                D3D11_BUFFER_DESC desc{
                    .ByteWidth = static_cast<UINT>(vertices->buffer.size()),
                    .Usage = D3D11_USAGE_DEFAULT,
                    .BindFlags = D3D11_BIND_VERTEX_BUFFER,
                    .CPUAccessFlags = 0,
                    .MiscFlags = 0,
                    .StructureByteStride = 0,
                };
                D3D11_SUBRESOURCE_DATA data{
                    .pSysMem = vertices->buffer.data(),
                    .SysMemPitch = vertices->stride,
                    .SysMemSlicePitch =
                        static_cast<UINT>(vertices->buffer.size()),
                };
                Gpu::ThrowIfFailed(
                    m_device->CreateBuffer(&desc, &data, &resource));
            }

            gpuMesh->VertexBuffer(resource);
        }

        // indices
        auto indices = sceneMesh->indices;
        ComPtr<ID3D11Buffer> resource;
        if (indices)
        {
            if (indices->isDynamic)
            {
                D3D11_BUFFER_DESC desc{
                    .ByteWidth = static_cast<UINT>(indices->buffer.size()),
                    .Usage = D3D11_USAGE_DYNAMIC,
                    .BindFlags = D3D11_BIND_INDEX_BUFFER,
                    .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
                    .MiscFlags = 0,
                    .StructureByteStride = 0,
                };
                Gpu::ThrowIfFailed(
                    m_device->CreateBuffer(&desc, nullptr, &resource));
            }
            else
            {
                D3D11_BUFFER_DESC desc{
                    .ByteWidth = static_cast<UINT>(indices->buffer.size()),
                    .Usage = D3D11_USAGE_DEFAULT,
                    .BindFlags = D3D11_BIND_INDEX_BUFFER,
                    .CPUAccessFlags = 0,
                    .MiscFlags = 0,
                    .StructureByteStride = 0,
                };
                D3D11_SUBRESOURCE_DATA data{
                    .pSysMem = indices->buffer.data(),
                    .SysMemPitch = indices->stride,
                    .SysMemSlicePitch =
                        static_cast<UINT>(indices->buffer.size()),
                };
                Gpu::ThrowIfFailed(
                    m_device->CreateBuffer(&desc, &data, &resource));
            }
            gpuMesh->IndexBuffer(resource);
        }

        m_meshMap.insert(std::make_pair(sceneMesh, gpuMesh));
        return gpuMesh;
    }

    std::unordered_map<framedata::FrameTexturePtr,
                       ComPtr<ID3D11ShaderResourceView>>
        m_textureMap;
    ComPtr<ID3D11ShaderResourceView>
    GetOrCreateTexture(const framedata::FrameTexturePtr &texture)
    {
        if (!texture)
        {
            return nullptr;
        }

        auto found = m_textureMap.find(texture);
        if (found != m_textureMap.end())
        {
            return found->second;
        }

        // create texture
        if (texture->Images.size() == 6)
        {
            throw std::runtime_error("not implemented");
            // cube
            // auto image = texture->Images.front();
            // auto resource = ResourceItem::CreateStaticCubemap(
            //     device, image->width, image->height,
            //     Utf8ToUnicode(image->name).c_str());
            // gpuTexture->ImageBuffer(resource);
            // gpuTexture->IsCubeMap = true;
            // TODO
            // uploader->EnqueueUpload(resource, image->buffer.data(),
            // (UINT)image->buffer.size(), image->width * 4);
        }
        else if (texture->Images.size() == 1)
        {
            ComPtr<ID3D11Texture2D> gpuTexture;
            // 2d
            auto image = texture->Images.front();
            D3D11_TEXTURE2D_DESC desc{
                .Width = static_cast<UINT>(image->width),
                .Height = static_cast<UINT>(image->height),
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                .SampleDesc =
                    {
                        1,
                        0,
                    },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                .CPUAccessFlags = 0,
                .MiscFlags = 0,
            };
            D3D11_SUBRESOURCE_DATA data{
                .pSysMem = image->buffer.data(),
                .SysMemPitch = static_cast<UINT>(image->width * 4),
                .SysMemSlicePitch = image->size(),
            };
            Gpu::ThrowIfFailed(
                m_device->CreateTexture2D(&desc, &data, &gpuTexture));
            ComPtr<ID3D11ShaderResourceView> srv;
            Gpu::ThrowIfFailed(m_device->CreateShaderResourceView(
                gpuTexture.Get(), nullptr, &srv));
            m_textureMap.insert(std::make_pair(texture, srv));
            return srv;
        }
        else
        {
            throw;
        }
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
    auto p = m_impl->ViewTexture().Get();
    return p;
}

void Renderer::ReleaseViewTexture(void *viewTexture) {}

void Renderer::View(const framedata::FrameData &framedata)
{
    m_impl->View(framedata);
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
