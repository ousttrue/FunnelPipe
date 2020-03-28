#pragma once
#include <hierarchy.h>
#include <filesystem>

class SceneManager
{
    hierarchy::Scene m_scene;
    hierarchy::SceneNodePtr m_selected;
    hierarchy::DrawList m_drawlist;

public:
    SceneManager(int argc, char **argv)
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

    void ImGui();

    const hierarchy::DrawList& Drawlist()const
    {
        return m_drawlist;
    }

    void OpenFile(const std::filesystem::path &path);
};
