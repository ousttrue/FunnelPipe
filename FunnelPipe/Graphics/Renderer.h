#pragma once
#include <memory>

namespace hierarchy
{
struct DrawList;
} // namespace hierarchy

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
    void View(const hierarchy::DrawList &drawlist);
};
