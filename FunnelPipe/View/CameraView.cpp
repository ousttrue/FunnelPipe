#include "CameraView.h"
#include "GuiView.h"
#include <ScreenState.h>
#include <frame_metrics.h>

CameraView::CameraView()
{
    m_camera.zNear = 0.01f;
}

bool CameraView::ImGui(const screenstate::ScreenState &state,
                       const Microsoft::WRL::ComPtr<ID3D12Resource> &texture,
                       const hierarchy::SceneNodePtr &selected,
                       framedata::FrameData *framedata)
{
    // view
    // imgui window for rendertarget. convert screenState for view
    screenstate::ScreenState viewState;
    bool isShowView = ::gui::View(framedata, state, texture, &viewState);

    framedata->ViewConstantBuffer.b0ScreenSize = {(float)viewState.Width, (float)viewState.Height};
    framedata->ViewConstantBuffer.b0Projection = m_camera.state.projection;
    framedata->ViewConstantBuffer.b0View = m_camera.state.view;
    framedata->ViewConstantBuffer.b0CameraPosition = m_camera.state.position;
    framedata->ViewConstantBuffer.fovY = m_camera.state.fovYRadians;

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

void CameraView::UpdateFrameData(framedata::FrameData *framedata)
{
    // gizmo
    if (framedata->ShowGizmo)
    {
        auto mesh = m_gizmo.GetMesh();
        if (mesh && !mesh->submeshes.empty())
        {
            auto shader = mesh->submeshes[0].material->shader;
            if (shader)
            {
                m_gizmoBuffer = m_gizmo.End();
                std::array<float, 16> matrix{
                    1, 0, 0, 0, //
                    0, 1, 0, 0, //
                    0, 0, 1, 0, //
                    0, 0, 0, 1, //
                };
                framedata::CBValue values[] =
                    {
                        {
                            .semantic = framedata::ConstantSemantics::NODE_WORLD,
                            .p = &matrix,
                            .size = sizeof(matrix),
                        }};
                framedata->PushCB(shader->VS.DrawCB(), values, _countof(values));
                framedata->Meshlist.push_back({
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
                });
                framedata->Drawlist.push_back({
                    .Mesh = mesh,
                    .Submesh = mesh->submeshes[0],
                });
            }
        }
    }
}
