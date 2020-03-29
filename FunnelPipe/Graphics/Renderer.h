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

    size_t ViewTextureID(size_t view);
    void View(const framedata::FrameData &drawlist);
};
