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

    void *ViewTexture(size_t view);
    void ReleaseViewTexture(void *viewTexture);
    void View(const framedata::FrameData &framedata);

    void *GetTexture(const framedata::FrameTexturePtr &texture);
};
