#include "CameraView.h"
#include "GuiView.h"
#include <ScreenState.h>
#include <hierarchy.h>
#include <frame_metrics.h>

CameraView::CameraView()
    : m_sceneView(new hierarchy::SceneView)
{
    m_camera.zNear = 0.01f;
    m_sceneView->ClearColor = {
        0.3f,
        0.4f,
        0.5f,
        1.0f,
    };
}

bool CameraView::ImGui(const screenstate::ScreenState &state, size_t textureID)
{
    // view
    // imgui window for rendertarget. convert screenState for view
    screenstate::ScreenState viewState;
    bool isShowView = ::gui::View(m_sceneView.get(), state, textureID, &viewState);

    m_sceneView->Width = viewState.Width;
    m_sceneView->Height = viewState.Height;
    m_sceneView->Projection = m_camera.state.projection;
    m_sceneView->View = m_camera.state.view;
    m_sceneView->CameraPosition = m_camera.state.position;
    m_sceneView->CameraFovYRadians = m_camera.state.fovYRadians;

    //
    // update camera
    //
    // if (selected != m_selected)
    // {
    //     if (selected)
    //     {
    //         m_camera.gaze = -selected->World().translation;
    //     }
    //     else
    //     {
    //         // m_camera->gaze = {0, 0, 0};
    //     }

    //     m_selected = selected;
    // }
    m_camera.Update(viewState);

    // //
    // // update gizmo
    // //
    // m_gizmo.Begin(viewState, m_camera.state);
    // if (selected)
    // {
    //     // if (selected->EnableGizmo())
    //     {
    //         auto parent = selected->Parent();
    //         m_gizmo.Transform(selected->ID(),
    //                           selected->Local(),
    //                           parent ? parent->World() : falg::Transform{});
    //     }
    // }

    return isShowView;
}
