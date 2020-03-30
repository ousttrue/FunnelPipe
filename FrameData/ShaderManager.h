#pragma once
#include "ShaderPass.h"

namespace framedata
{

class ShaderManager
{
public:
    static ShaderManager &Instance();

    ShaderPassPtr GltfUnlit();
    ShaderPassPtr GltfPBR();
    ShaderPassPtr Gizmo();
    ShaderPassPtr Grid();
};

} // namespace framedata
