#include "SceneManager.h"
#include <imgui.h>
#include <functional>
#include <nameof.hpp>

SceneManager::SceneManager(int argc, char **argv)
{
    auto path = std::filesystem::current_path();
    if (argc > 1)
    {
        path = argv[1];
    }
    framedata::ShaderManager::Instance().watch(path);

    {
        auto node = hierarchy::SceneNode::Create("grid");
        node->Mesh(framedata::CreateGrid());
        m_scene.gizmoNodes.push_back(node);
    }

    if (argc > 2)
    {
        auto model = hierarchy::SceneModel::LoadFromPath(argv[2]);
        if (model)
        {
            m_scene.model = model;
        }
    }
}

void SceneManager::DrawNode(const hierarchy::SceneNodePtr &node)
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
    if (node == m_selected.lock())
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    auto isOpen = ImGui::TreeNodeEx(node->Name().c_str(), flags);
    if (ImGui::IsItemClicked())
    {
        m_selected = node;
    }

    if (isOpen)
    {
        // children
        for (int i = 0; i < childCount; ++i)
        {
            DrawNode(children[i]);
        }
        ImGui::TreePop();
    }

    ImGui::PopID();
}

static void MaterialList(const hierarchy::SceneModelPtr &model)
{
    int i = 0;
    for (auto &material : model->materials)
    {
        if (i)
        {
            ImGui::Separator();
        }
        ImGui::Text("%d %s", i, material->name.c_str());

        auto shader = material->shaderSource->Compiled();
        if (shader)
        {
            ImGui::Text("shader: %s", material->shaderSource->name().c_str());
        }

        ImGui::Text("alphaMode: %s", nameof::nameof_enum(material->alphaMode).data());
        // if (material->alphaMode == framedata::AlphaMode::Mask)
        {
            ImGui::Text("alphaCutoff: %f", material->alphaCutoff);
        }

        if (material->colorImage)
        {
            ImGui::Text("colorImage: %s: %d x %d",
                        material->colorImage->name.c_str(),
                        material->colorImage->width, material->colorImage->height);
        }
        else
        {
            ImGui::Text("colorImage: null");
        }
        ImGui::ColorEdit4("color", material->color.data());

        ++i;
    }
}

static void MeshList(const hierarchy::SceneModelPtr &model)
{
    // ImGui::Text("With border:");
    ImGui::Columns(4, "mycolumns"); // 4-ways, with border
    ImGui::Separator();
    ImGui::Text("ID");
    ImGui::NextColumn();
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("Vertices");
    ImGui::NextColumn();
    ImGui::Text("Submeshes");
    ImGui::NextColumn();
    ImGui::Separator();
    // const char *names[3] = {"One", "Two", "Three"};
    // const char *paths[3] = {"/path/one", "/path/two", "/path/three"};
    static int selected = -1;
    int i = 0;
    for (auto &mesh : model->meshes)
    {
        char label[32];
        sprintf(label, "%04d", i);
        if (ImGui::Selectable(label, selected == i, ImGuiSelectableFlags_SpanAllColumns))
            selected = i;
        bool hovered = ImGui::IsItemHovered();

        // auto &mesh = model->meshes[i];
        ImGui::NextColumn();
        ImGui::Text(mesh->name.c_str());
        ImGui::NextColumn();
        ImGui::Text("%d", mesh->vertices->Count());
        ImGui::NextColumn();
        ImGui::Text("%d", mesh->submeshes.size());
        ImGui::NextColumn();

        ++i;
    }
    ImGui::Columns(1);
    ImGui::Separator();
}

void SceneManager::ImGui()
{
    // scene tree
    ImGui::Begin("Model");
    ImGui::SetWindowSize(ImVec2(256, 512), ImGuiCond_FirstUseEver);

    if (m_scene.model)
    {
        if (ImGui::CollapsingHeader("Materials"))
        {
            MaterialList(m_scene.model);
        }
        if (ImGui::CollapsingHeader("Meshes"))
        {
            MeshList(m_scene.model);
        }
        if (ImGui::CollapsingHeader("Scene"))
        {
            DrawNode(m_scene.model->root);
        }
    }

    ImGui::End();
}

void SceneManager::OpenFile(const std::filesystem::path &path)
{
    auto model = hierarchy::SceneModel::LoadFromPath(path);

    if (model)
    {
        m_scene.model = model;
    }
}

namespace hierarchy
{

using FilterFunc = std::function<bool(const framedata::FrameMaterialPtr &)>;

void TraverseMesh(framedata::FrameData *framedata, const std::shared_ptr<SceneNode> &node, const FilterFunc &filter)
{
    auto mesh = node->Mesh();
    if (mesh)
    {
        framedata->Meshlist.push_back({
            .Mesh = mesh,
        });
        if (node->skin)
        {
            framedata->Meshlist.back().Skin = {
                node->skin->cpuSkiningBuffer.data(),
                (uint32_t)node->skin->cpuSkiningBuffer.size(),
                mesh->vertices->stride,
            };
        }

        for (auto &submesh : mesh->submeshes)
        {
            auto &material = submesh.material;
            if (filter(material))
            {
                auto shader = material->shaderSource->Compiled();
                if (shader)
                {
                    auto m = node->World().RowMatrix();
                    framedata::CBValue values[] = {
                        {.semantic = framedata::ConstantSemantics::NODE_WORLD,
                         .p = &m,
                         .size = sizeof(m)}};
                    framedata->PushCB(shader->VS.DrawCB(), values, _countof(values));
                    framedata->Drawlist.push_back({
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
        TraverseMesh(framedata, *child, filter);
    }
}

static void UpdateFrameDataIf(framedata::FrameData *framedata, const Scene *scene, const FilterFunc &filter)
{
    if (framedata->ShowGrid)
    {
        for (auto &node : scene->gizmoNodes)
        {
            TraverseMesh(framedata, node, filter);
        }
    }
    // if (view->ShowVR)
    // {
    //     for (auto &node : scene->vrNodes)
    //     {
    //         TraverseMesh(&view->Drawlist, node, filter);
    //     }
    // }
    // for (auto &node : scene->sceneNodes)
    if (scene->model)
    {
        TraverseMesh(framedata, scene->model->root, filter);
    }
}
} // namespace hierarchy

void SceneManager::UpdateFrameData(framedata::FrameData *framedata)
{
    m_scene.Update();

    //
    // mesh
    //
    // Opaque
    hierarchy::UpdateFrameDataIf(framedata, &m_scene, [](const framedata::FrameMaterialPtr &m) {
        return m->alphaMode != framedata::AlphaMode::Blend;
    });
    // AlphaBlend
    hierarchy::UpdateFrameDataIf(framedata, &m_scene, [](const framedata::FrameMaterialPtr &m) {
        return m->alphaMode == framedata::AlphaMode::Blend;
    });
}
