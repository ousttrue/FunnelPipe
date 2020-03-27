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

    const OrbitCamera *Camera() const
    {
        return &m_camera;
    }

    int GizmoNodeID() const
    {
        return m_gizmo.GetNodeID();
    }

    hierarchy::SceneMeshPtr GizmoMesh() const
    {
        return m_gizmo.GetMesh();
    }

    gizmesh::GizmoSystem::Buffer GizmoBuffer()
    {
        return m_gizmo.End();
    }

    void OnFrame();

    void Update3DView(const screenstate::ScreenState &viewState, const hierarchy::SceneNodePtr &selected);
};
