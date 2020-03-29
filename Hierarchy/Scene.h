#pragma once
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include "SceneNode.h"
#include "SceneModel.h"
#include "FrameMaterial.h"
#include "FrameMesh.h"

namespace hierarchy
{
class Scene
{

public:
    std::vector<SceneNodePtr> gizmoNodes;
    std::vector<SceneNodePtr> vrNodes;
    // std::vector<SceneNodePtr> sceneNodes;

    SceneModelPtr model;

    Scene();
    void Update();
};

} // namespace hierarchy
