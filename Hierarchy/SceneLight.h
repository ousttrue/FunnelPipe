#pragma once
#include <array>
#include <memory>
#include <FrameData.h>

namespace hierarchy
{

class SceneDirectionalLight
{
public:
    std::array<float, 3> LightDirection = {0, -1, 0};
    std::array<float, 3> LightColor = {1, 1, 1};
};
using SceneDirectionalLightPtr = std::shared_ptr<SceneDirectionalLight>;

class SceneCubemap
{
public:
    std::array<framedata::FrameImagePtr, 6> Images = {};
};
using SceneCubemapPtr = std::shared_ptr<SceneCubemap>;

} // namespace hierarchy
