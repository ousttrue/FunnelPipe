#pragma once
#include <FrameData.h>
#include "SceneMeshSkin.h"
#include "SceneNode.h"
#include <vector>

namespace hierarchy
{

struct SceneModel
{
    std::string name;
    std::vector<framedata::FrameTexturePtr> textures;
    std::vector<framedata::FrameMaterialPtr> materials;
    std::vector<framedata::FrameMeshPtr> meshes;
    std::vector<SceneMeshSkinPtr> skins;
    std::vector<SceneNodePtr> nodes;

    SceneNodePtr root;

    static std::shared_ptr<SceneModel> LoadFromPath(const std::filesystem::path &path);
    static std::shared_ptr<SceneModel> LoadGlbBytes(const uint8_t *p, int size);
};
using SceneModelPtr = std::shared_ptr<SceneModel>;

} // namespace hierarchy
