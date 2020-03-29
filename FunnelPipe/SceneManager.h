#pragma once
#include <hierarchy.h>
#include <FrameData.h>
#include <filesystem>

class SceneManager
{
    hierarchy::Scene m_scene;

public:
    SceneManager(int argc, char **argv);
    void ImGui();
    void OpenFile(const std::filesystem::path &path);
    void UpdateFrameData(framedata::FrameData *framedata);
    hierarchy::SceneNodePtr Selected() const { return m_scene.selected.lock(); }
};
