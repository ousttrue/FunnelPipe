#include <memory>
#include <unordered_map>
#include <hierarchy.h>
#include <d3d12.h>
#include <wrl/client.h>

namespace Gpu::dx12
{

class SceneMapper
{
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    std::unique_ptr<class Uploader> m_uploader;
    std::unordered_map<framedata::FrameMeshPtr, std::shared_ptr<class Mesh>> m_meshMap;
    std::unordered_map<size_t, std::shared_ptr<class RenderTargetChain>> m_renderTargetMap;

public:
    SceneMapper();
    class Uploader *GetUploader() { return m_uploader.get(); }
    void Initialize(const ComPtr<ID3D12Device> &device);
    void Update(const ComPtr<ID3D12Device> &device);
    std::shared_ptr<class Mesh> GetOrCreate(const ComPtr<ID3D12Device> &device,
                                            const framedata::FrameMeshPtr &model);
    std::shared_ptr<class RenderTargetChain> GetOrCreateRenderTarget(size_t id);
};

} // namespace Gpu::dx12
