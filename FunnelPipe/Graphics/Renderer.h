#pragma once
#include <memory>
#include <FrameData.h>

class Renderer
{
    class Impl *m_impl = nullptr;

public:
    Renderer(int maxModelCount);
    ~Renderer();
    void Initialize(void *hwnd);

    void BeginFrame(void *hwnd, int width, int height);
    void EndFrame();

    ID3D12Resource* ViewTexture(size_t view);
    void View(const framedata::FrameData &framedata);

    void *GetTexture(const framedata::FrameTexturePtr &texture);
};
