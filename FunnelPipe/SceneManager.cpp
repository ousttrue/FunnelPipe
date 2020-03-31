#include "SceneManager.h"
#include <Gpu.h>
#include <imgui.h>
#include <functional>
#include <sstream>
#include <memory>
#include <nameof.hpp>

SceneManager::SceneManager(int argc, char **argv)
{
    // watch file path
    auto path = std::filesystem::current_path();
    if (argc > 1)
    {
        path = argv[1];
    }
    // auto callback = [](const std::wstring &fileName, int action) {
    //     framedata::ShaderManager::Instance().OnFile(fileName, action);
    // };
    framedata::DirectoryWatcher::Instance().Watch(path);

    // grid
    {
        auto node = hierarchy::SceneNode::Create("grid");
        node->Mesh(framedata::CreateGrid());
        m_scene.gizmoNodes.push_back(node);
    }

    // load
    if (argc > 2)
    {
        auto model = hierarchy::SceneModel::LoadFromPath(argv[2]);
        if (model)
        {
            m_scene.model = model;
        }
    }
}

SceneManager::~SceneManager()
{
    framedata::DirectoryWatcher::Instance().Stop();
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

static void MaterialList(const hierarchy::SceneModelPtr &model,
                         const SceneManager::GetTextureFunc &getTexture)
{
    int i = 0;
    for (auto &material : model->materials)
    {
        std::stringstream ss;
        ss << "[" << i++ << "] " << material->Name;
        if (ImGui::TreeNode(ss.str().c_str()))
        {
            // auto shader = material->shader;
            // if (shader)
            // {
            //     ImGui::Text("shader: %s", material->shader->Name().c_str());
            // }

            ImGui::Text("alphaMode: %s", nameof::nameof_enum(material->AlphaMode).data());
            // if (material->alphaMode == framedata::AlphaMode::Mask)
            {
                ImGui::Text("alphaCutoff: %f", material->AlphaCutoff);
            }

            auto colorTexture = material->ColorTexture;
            if (colorTexture)
            {
                auto colorImage = colorTexture->Image;
                ImGui::Text("colorImage: %s: %d x %d",
                            colorImage->name.c_str(),
                            colorImage->width, colorImage->height);
                auto texture = getTexture(colorTexture);
                texture->AddRef();
                ImGui::Image((ImTextureID)texture.Get(), ImVec2(150, 150));
            }
            else
            {
                ImGui::Text("colorImage: null");
            }
            ImGui::ColorEdit4("color", material->Color.data());

            ImGui::TreePop();
        }
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

void SceneManager::ImGui(const GetTextureFunc &getTexture)
{
    // scene tree
    ImGui::Begin("Model");
    ImGui::SetWindowSize(ImVec2(256, 512), ImGuiCond_FirstUseEver);

    if (m_scene.model)
    {
        if (ImGui::CollapsingHeader("Materials"))
        {
            MaterialList(m_scene.model, getTexture);
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

void SceneManager::UpdateFrameData(framedata::FrameData *framedata)
{
    m_scene.Update();
    hierarchy::FrameDataBuilder builder(m_scene);
    builder.UpdateFrameData(framedata);
}
