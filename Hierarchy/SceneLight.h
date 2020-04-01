#pragma once
#include <array>
#include <memory>

namespace hierarchy
{

class SceneLight
{
public:
    std::array<float, 3> LightDirection = {0, -1, 0};
    std::array<float, 3> LightColor = {1, 1, 1};
};
using SceneLightPtr = std::shared_ptr<SceneLight>;

} // namespace hierarchy
