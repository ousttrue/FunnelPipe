#include "SceneManager.h"
#include <imgui.h>

SceneManager::SceneManager(int argc, char **argv)
{
    auto path = std::filesystem::current_path();
    if (argc > 1)
    {
        path = argv[1];
    }
    hierarchy::ShaderManager::Instance().watch(path);

    {
        auto node = hierarchy::SceneNode::Create("grid");
        node->Mesh(hierarchy::CreateGrid());
        m_scene.gizmoNodes.push_back(node);
    }

    if (argc > 2)
    {
        auto model = hierarchy::SceneModel::LoadFromPath(argv[2]);
        if (model)
        {
            m_scene.sceneNodes.push_back(model->root);
        }
    }
}

static void DrawNode(const hierarchy::SceneNodePtr &node, hierarchy::Scene *scene)
{
    int childCount;
    auto children = node->GetChildren(&childCount);
    ImGui::PushID(node->ID());
    auto flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    flags |= ImGuiTreeNodeFlags_DefaultOpen;
    if (childCount == 0)
    {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (node == scene->selected.lock())
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    auto isOpen = ImGui::TreeNodeEx(node->Name().c_str(), flags);
    if (ImGui::IsItemClicked())
    {
        scene->selected = node;
    }

    if (isOpen)
    {
        // children
        for (int i = 0; i < childCount; ++i)
        {
            DrawNode(children[i], scene);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void SceneManager::ImGui()
{
    // scene tree
    ImGui::Begin("scene graph");
    ImGui::SetWindowSize(ImVec2(256, 512), ImGuiCond_FirstUseEver);

    for (auto &node : m_scene.sceneNodes)
    {
        DrawNode(node, &m_scene);
    }

    ImGui::End();
}

void SceneManager::OpenFile(const std::filesystem::path &path)
{
    auto model = hierarchy::SceneModel::LoadFromPath(path);

    if (model)
    {
        m_scene.sceneNodes.clear();
        m_scene.sceneNodes.push_back(model->root);
    }
}

void SceneManager::UpdateDrawlist(hierarchy::DrawList *drawlist)
{
}
