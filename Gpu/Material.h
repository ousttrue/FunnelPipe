#include "Helper.h"
#include <memory>
#include <Drawlist.h>

namespace Gpu::dx12
{

class Material : NonCopyable
{
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    int m_lastGeneration = -1;

public:
    bool Initialize(const ComPtr<ID3D12Device> &device, const hierarchy::SceneMaterialPtr &material)
    {
        return Initialize(device, m_rootSignature, material);
    }

    bool Initialize(const ComPtr<ID3D12Device> &device,
                    const ComPtr<ID3D12RootSignature> &rootSignature,
                    const hierarchy::SceneMaterialPtr &material);
    bool Set(const ComPtr<ID3D12GraphicsCommandList> &commandList);
};

} // namespace Gpu::dx12
