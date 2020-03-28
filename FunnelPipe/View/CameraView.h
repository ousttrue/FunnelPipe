#pragma once
#include "Gizmo.h"
#include <OrbitCamera.h>
#include <hierarchy.h>

///
/// * 3D View
/// * Camera
/// * Gizmo
///
class CameraView
{
    gizmesh::GizmoSystem::Buffer m_gizmoBuffer;
    std::shared_ptr<hierarchy::SceneView> m_sceneView;
    size_t m_viewTextureID = 0;

    OrbitCamera m_camera;
    Gizmo m_gizmo;
    hierarchy::SceneNodePtr m_selected;

public:
    CameraView();
    const std::shared_ptr<hierarchy::SceneView> &SceneView() const { return m_sceneView; }
    bool ImGui(const screenstate::ScreenState &state, size_t textureID,
               const hierarchy::SceneNodePtr &selected);
    void UpdateDrawlist(hierarchy::DrawList *drawlist);
};
