#include "SceneManager.h"
#include <imgui.h>
#include <functional>

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

namespace hierarchy
{

using FilterFunc = std::function<bool(const SceneMaterialPtr &)>;

void TraverseMesh(DrawList *drawlist, const std::shared_ptr<SceneNode> &node, const FilterFunc &filter)
{
    auto mesh = node->Mesh();
    if (mesh)
    {

        // auto &submeshes = mesh->submeshes;
        for (auto &submesh: mesh->submeshes)
        {
            auto &material = submesh.material;
            if (filter(material))
            {
                auto shader = material->shader->Compiled();
                if (shader)
                {
                    auto m = node->World().RowMatrix();
                    CBValue values[] = {
                        {.semantic = ConstantSemantics::NODE_WORLD,
                         .p = &m,
                         .size = sizeof(m)}};
                    drawlist->PushCB(shader->VS.DrawCB(), values, _countof(values));
                    drawlist->Items.push_back({
                        .Mesh = mesh,
                        .Submesh = submesh,
                    });
                }
            }
        }
    }

    int count;
    auto child = node->GetChildren(&count);
    for (int i = 0; i < count; ++i, ++child)
    {
        TraverseMesh(drawlist, *child, filter);
    }
}

static void UpdateDrawListIf(DrawList *drawlist, const Scene *scene, bool showGrid, const FilterFunc &filter)
{
    if (showGrid)
    {
        for (auto &node : scene->gizmoNodes)
        {
            TraverseMesh(drawlist, node, filter);
        }
    }
    // if (view->ShowVR)
    // {
    //     for (auto &node : scene->vrNodes)
    //     {
    //         TraverseMesh(&view->Drawlist, node, filter);
    //     }
    // }
    for (auto &node : scene->sceneNodes)
    {
        TraverseMesh(drawlist, node, filter);
    }
}
} // namespace hierarchy

void SceneManager::UpdateDrawlist(hierarchy::DrawList *drawlist,
                                  bool showGrid)
{
    m_scene.Update();

    //
    // mesh
    //
    // Opaque
    hierarchy::UpdateDrawListIf(drawlist, &m_scene, showGrid, [](const hierarchy::SceneMaterialPtr &m) {
        return m->alphaMode != hierarchy::AlphaMode::Blend;
    });
    // AlphaBlend
    hierarchy::UpdateDrawListIf(drawlist, &m_scene, showGrid, [](const hierarchy::SceneMaterialPtr &m) {
        return m->alphaMode == hierarchy::AlphaMode::Blend;
    });
}
