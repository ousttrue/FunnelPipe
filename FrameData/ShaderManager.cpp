#include "ShaderManager.h"
#include "DirectoryWatcher.h"

namespace framedata
{

ShaderManager &ShaderManager::Instance()
{
    static ShaderManager s_instance;
    return s_instance;
}

ShaderPassPtr ShaderManager::GltfUnlit()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = std::make_shared<ShaderPass>("unlit");
        s_shader->VS = std::make_shared<framedata::VertexShader>("gltf@vs");
        s_shader->VS->Compile(framedata::DirectoryWatcher::Instance().Get(L"gltf_unlit.hlsl")->String());
        s_shader->PS = std::make_shared<framedata::PixelShader>("gltf@ps");
        s_shader->PS->Compile(framedata::DirectoryWatcher::Instance().Get(L"gltf_unlit.hlsl")->String());
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::GltfPBR()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = std::make_shared<ShaderPass>("pbr");
        s_shader->VS = std::make_shared<framedata::VertexShader>("gltf@vs");
        s_shader->VS->Compile(framedata::DirectoryWatcher::Instance().Get(L"gltf_standard.hlsl")->String());
        s_shader->PS = std::make_shared<framedata::PixelShader>("gltf@ps");
        s_shader->PS->Compile(framedata::DirectoryWatcher::Instance().Get(L"gltf_standard.hlsl")->String());
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::Gizmo()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = std::make_shared<ShaderPass>("gizmo");
        s_shader->VS = std::make_shared<framedata::VertexShader>("gizmo@vs");
        s_shader->VS->Compile(framedata::DirectoryWatcher::Instance().Get(L"gizmo.hlsl")->String());
        s_shader->PS = std::make_shared<framedata::PixelShader>("gizmo@ps");
        s_shader->PS->Compile(framedata::DirectoryWatcher::Instance().Get(L"gizmo.hlsl")->String());
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::Grid()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = std::make_shared<ShaderPass>("grid");
        s_shader->VS = std::make_shared<VertexShader>("grid.hlsl@vs");
        s_shader->VS->Compile(DirectoryWatcher::Instance().Get(L"grid.hlsl")->String());
        s_shader->PS = std::make_shared<PixelShader>("grid.hlsl@ps");
        s_shader->PS->Compile(DirectoryWatcher::Instance().Get(L"grid.hlsl")->String());
    }
    return s_shader;
}

} // namespace framedata