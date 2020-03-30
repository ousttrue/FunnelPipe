#pragma once
#include "Shader.h"

namespace framedata
{

struct ShaderPass
{
    std::string Name;
    VertexShaderPtr VS;
    PixelShaderPtr PS;

    ShaderPass() = default;

    ShaderPass(const std::string &name)
        : Name(name)
    {
    }
};
using ShaderPassPtr = std::shared_ptr<ShaderPass>;

} // namespace framedata
