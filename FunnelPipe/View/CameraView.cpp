#include "CameraView.h"
#include "GuiView.h"
#include <ScreenState.h>
#include <frame_metrics.h>

CameraView::CameraView()
{
    m_camera.zNear = 0.01f;
}

bool CameraView::ImGui(const screenstate::ScreenState &state, size_t textureID,
                       const hierarchy::SceneNodePtr &selected,
                       framedata::FrameData *drawlist)
{
    // view
    // imgui window for rendertarget. convert screenState for view
    screenstate::ScreenState viewState;
    bool isShowView = ::gui::View(drawlist, state, textureID, &viewState);

    drawlist->ViewConstantBuffer.b0ScreenSize = {(float)viewState.Width, (float)viewState.Height};
    drawlist->ViewConstantBuffer.b0Projection = m_camera.state.projection;
    drawlist->ViewConstantBuffer.b0View = m_camera.state.view;
    drawlist->ViewConstantBuffer.b0CameraPosition = m_camera.state.position;
    drawlist->ViewConstantBuffer.fovY = m_camera.state.fovYRadians;

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

void CameraView::UpdateDrawlist(framedata::FrameData *drawlist)
{
    // gizmo
    if (drawlist->ShowGizmo)
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
                framedata::CBValue values[] =
                    {
                        {
                            .semantic = hierarchy::ConstantSemantics::NODE_WORLD,
                            .p = &matrix,
                            .size = sizeof(matrix),
                        }};
                drawlist->PushCB(shader->VS.DrawCB(), values, _countof(values));
                drawlist->Items.push_back({
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
                    .Submesh = mesh->submeshes[0],
                });
            }
        }
    }
}
