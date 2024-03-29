#pragma once
#include <hierarchy.h>
#include <FrameData.h>
#include <filesystem>
#include <Gpu.h>
#include <functional>

class SceneManager
{
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

    hierarchy::Scene m_scene;
    hierarchy::FrameDataBuilder m_builder;
    // single selection
    std::weak_ptr<hierarchy::SceneNode> m_selected;

  public:
    using GetTextureFunc =
        std::function<void *(const framedata::FrameTexturePtr &)>;
    SceneManager(int argc, char **argv);
    ~SceneManager();
    void ImGui(const GetTextureFunc &getTexture);
    void OpenFile(const std::filesystem::path &path);
    void UpdateFrameData(framedata::FrameData *framedata);
    hierarchy::SceneNodePtr Selected() const { return m_selected.lock(); }

  private:
    void DrawNode(const hierarchy::SceneNodePtr &node);
};
