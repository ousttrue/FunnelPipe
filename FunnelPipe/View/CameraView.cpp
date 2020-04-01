#include "CameraView.h"
#include "GuiView.h"
#include <ScreenState.h>
#include <frame_metrics.h>

CameraView::CameraView()
    : m_light(new hierarchy::SceneLight)
{
    m_camera.zNear = 0.01f;
    m_light->LightDirection = falg::Normalize(std::array<float, 3>{1, 2, 3});
    m_light->LightColor = {3, 3, 3};
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

    auto &cb = framedata->ViewConstantBuffer;

    cb.b0ScreenSize = {(float)viewState.Width, (float)viewState.Height};
    cb.b0Projection = m_camera.state.projection;
    cb.b0View = m_camera.state.view;
    cb.b0CameraPosition = m_camera.state.position;
    cb.fovY = m_camera.state.fovYRadians;
    cb.b0LightColor = m_light->LightColor;
    cb.b0LightDir = m_light->LightDirection;

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
            m_gizmoBuffer = m_gizmo.End();
            auto shader = mesh->submeshes[0].material->Shader;
            if (shader)
            {
                auto vs = shader->VS;
                if (vs)
                {
                    auto cb = vs->CB(1);
                    framedata->PushCB(cb);
                    framedata->SetCBVariable(cb,
                                             framedata::ConstantSemantics::NODE_WORLD,
                                             std::array<float, 16>{
                                                 1, 0, 0, 0, //
                                                 0, 1, 0, 0, //
                                                 0, 0, 1, 0, //
                                                 0, 0, 0, 1, //
                                             });
                }
                auto ps = shader->PS;
                if (ps)
                {
                    auto cb = ps->CB(1);
                    framedata->PushCB(cb);
                    framedata->SetCBVariable(cb,
                                             framedata::ConstantSemantics::NODE_WORLD,
                                             std::array<float, 16>{
                                                 1, 0, 0, 0, //
                                                 0, 1, 0, 0, //
                                                 0, 0, 1, 0, //
                                                 0, 0, 0, 1, //
                                             });
                }
            }
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
            framedata->PushDraw(mesh, 0);
        }
    }
}
