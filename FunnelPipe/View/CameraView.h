#pragma once
#include "Gizmo.h"
#include <OrbitCamera.h>
#include <hierarchy.h>

class CameraView
{
    hierarchy::Scene m_scene;
    gizmesh::GizmoSystem::Buffer m_gizmoBuffer;
    std::shared_ptr<hierarchy::SceneView> m_sceneView;
    size_t m_viewTextureID = 0;

    OrbitCamera m_camera;
    Gizmo m_gizmo;
    hierarchy::SceneNodePtr m_selected;

public:
    CameraView(int argc, char **argv);

    const std::shared_ptr<hierarchy::SceneView> &SceneView() const { return m_sceneView; }

    const OrbitCamera *Camera() const
    {
        return &m_camera;
    }

    // int GizmoNodeID() const
    // {
    //     return m_gizmo.GetNodeID();
    // }

    // hierarchy::SceneMeshPtr GizmoMesh() const
    // {
    //     return m_gizmo.GetMesh();
    // }

    // gizmesh::GizmoSystem::Buffer GizmoBuffer()
    // {
    //     return m_gizmo.End();
    // }

    bool OnFrame(const screenstate::ScreenState &state, size_t textureID,
                 screenstate::ScreenState *viewState);

    void Update3DView(const screenstate::ScreenState &viewState);
};
