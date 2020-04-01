#pragma once
#include "Gizmo.h"
#include <OrbitCamera.h>
#include <hierarchy.h>
#include <FrameData.h>

///
/// * 3D View
/// * Camera
/// * Gizmo
///
class CameraView
{
    gizmesh::GizmoSystem::Buffer m_gizmoBuffer;
    size_t m_viewTextureID = 0;

    OrbitCamera m_camera;
    Gizmo m_gizmo;
    hierarchy::SceneNodePtr m_selected;
    hierarchy::SceneDirectionalLightPtr m_light;
    hierarchy::SceneCubemapPtr m_cubemap;

public:
    CameraView();
    bool ImGui(const screenstate::ScreenState &state,
               ID3D12Resource *resource,
               const hierarchy::SceneNodePtr &selected,
               framedata::FrameData *framedata);
    void UpdateFrameData(framedata::FrameData *framedata);
};
