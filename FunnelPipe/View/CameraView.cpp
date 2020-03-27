#include "CameraView.h"
#include <ScreenState.h>
#include <hierarchy.h>
#include <frame_metrics.h>

CameraView::CameraView(int argc, char **argv)
    : m_sceneView(new hierarchy::SceneView)
{
    m_camera.zNear = 0.01f;
    m_sceneView->ClearColor = {
        0.3f,
        0.4f,
        0.5f,
        1.0f,
    };

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

void CameraView::OnFrame()
{
    // bool isShowView = false;
    screenstate::ScreenState viewState;
    {

        // // view
        // // imgui window for rendertarget. convert screenState for view
        // isShowView = m_imgui.View(m_sceneView.get(), state, viewTextureID,
        //                           &viewState);
    }
}

void CameraView::Update3DView(const screenstate::ScreenState &viewState, const hierarchy::SceneNodePtr &selected)
{
    //
    // update camera
    //
    if (selected != m_selected)
    {
        if (selected)
        {
            m_camera.gaze = -selected->World().translation;
        }
        else
        {
            // m_camera->gaze = {0, 0, 0};
        }

        m_selected = selected;
    }
    m_camera.Update(viewState);

    //
    // update gizmo
    //
    m_gizmo.Begin(viewState, m_camera.state);
    if (selected)
    {
        // if (selected->EnableGizmo())
        {
            auto parent = selected->Parent();
            m_gizmo.Transform(selected->ID(),
                              selected->Local(),
                              parent ? parent->World() : falg::Transform{});
        }
    }
}
