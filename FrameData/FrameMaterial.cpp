#include "FrameMaterial.h"
#include "ShaderManager.h"

namespace framedata
{

std::shared_ptr<FrameMaterial> FrameMaterial::Create()
{
    auto material = FrameMaterialPtr(new FrameMaterial);
    material->shader = ShaderManager::Instance().getDefault();
    return material;
}

} // namespace framedata