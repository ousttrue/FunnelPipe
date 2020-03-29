#include "Texture.h"
#include "CommandList.h"
#include "ResourceItem.h"

namespace Gpu::dx12
{

const Microsoft::WRL::ComPtr<ID3D12Resource> &Texture::Resource() const
{
    return m_imageBuffer->Resource();
}

std::pair<bool, std::function<void()>> Texture::IsDrawable(const ComPtr<ID3D12GraphicsCommandList> &commandList)
{
    std::function<void()> callback;

    if (!m_imageBuffer)
    {
        return {false, callback};
    }

    auto state = m_imageBuffer->State();
    if (state.State == D3D12_RESOURCE_STATE_COPY_DEST)
    {
        if (state.Upload == UploadStates::Uploaded)
        {
            callback = m_imageBuffer->EnqueueTransition(commandList, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }
    }

    if (!state.Drawable())
    {
        return {false, callback};
    }

    return {true, callback};
}

} // namespace Gpu::dx12