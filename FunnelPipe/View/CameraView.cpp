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

bool CameraView::ImGui(const screenstate::ScreenState &state, size_t textureID,
                       const hierarchy::SceneNodePtr &selected)
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

    return isShowView;
}

void CameraView::UpdateDrawlist(hierarchy::DrawList *drawlist)
{
    // gizmo
    if (m_sceneView->ShowGizmo)
    {
        auto mesh = m_gizmo.GetMesh();
        if (mesh)
        {
            auto shader = mesh->submeshes[0].material->shader->Compiled();
            if (shader)
            {
                m_gizmoBuffer = m_gizmo.End();
                std::array<float, 16> matrix{
                    1, 0, 0, 0, //
                    0, 1, 0, 0, //
                    0, 0, 1, 0, //
                    0, 0, 0, 1, //
                };
                hierarchy::CBValue values[] =
                    {
                        {
                            .semantic = hierarchy::ConstantSemantics::NODE_WORLD,
                            .p = &matrix,
                            .size = sizeof(matrix),
                        }};
                m_sceneView->Drawlist.PushCB(shader->VS.DrawCB(), values, _countof(values));
                m_sceneView->Drawlist.Items.push_back({
                    .Mesh = mesh,
                    .Vertices = {
                        .Ptr = m_gizmoBuffer.pVertices,
                        .Size = m_gizmoBuffer.verticesBytes,
                        .Stride = m_gizmoBuffer.vertexStride,
                    },
                    .Indices = {
                        .Ptr = m_gizmoBuffer.pIndices,
                        .Size = m_gizmoBuffer.indicesBytes,
                        .Stride = m_gizmoBuffer.indexStride,
                    },
                    .SubmeshIndex = 0,
                });
            }
        }
    }
}
