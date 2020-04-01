#include "ShaderManager.h"
#include "DirectoryWatcher.h"

namespace framedata
{

ShaderManager &ShaderManager::Instance()
{
    static ShaderManager s_instance;
    return s_instance;
}

static ShaderPassPtr CreateShader(
    const std::string &name,
    const std::wstring &vs, const std::string &vsEntryPoint, const D3D_SHADER_MACRO *vsDefine,
    const std::wstring &ps, const std::string &psEntryPoint, const D3D_SHADER_MACRO *psDefine)
{
    auto shader = std::make_shared<ShaderPass>(name);
    {
        shader->VS = std::make_shared<framedata::VertexShader>(name + "@vs");
        auto watcher = framedata::DirectoryWatcher::Instance().Get(vs);
        auto source = watcher->String();
        if (source.empty())
        {
            throw;
        }
        if (!shader->VS->Compile(source, vsEntryPoint, vsDefine))
        {
            throw;
        }
    }
    {
        shader->PS = std::make_shared<framedata::PixelShader>(name + "@ps");
        auto watcher = framedata::DirectoryWatcher::Instance().Get(ps);
        auto source = watcher->String();
        if (source.empty())
        {
            throw;
        }
        if (!shader->PS->Compile(source, psEntryPoint, psDefine))
        {
            throw;
        }
    }
    return shader;
}

ShaderPassPtr ShaderManager::GltfUnlit()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = CreateShader(
            "unlit",
            L"gltf_unlit.hlsl", "VSMain", nullptr,
            L"gltf_unlit.hlsl", "PSMain", nullptr);
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::GltfPBR()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        D3D_SHADER_MACRO vsDefines[] = {
            {"NORMALS", "1"},
            {"UV", "1"},
            {nullptr, nullptr},
        };
        D3D_SHADER_MACRO psDefines[] = {
            {"NORMALS", "1"},
            {"UV", "1"},
            {"HAS_BASECOLORMAP", "1"},
            {"HAS_EMISSIVEMAP", "1"},
            {"HAS_OCCLUSIONMAP", "1"},
            {"HAS_NORMALMAP", "1"},
            {"HAS_METALROUGHNESSMAP", "1"},
            {nullptr, nullptr},
        };
        s_shader = CreateShader(
            "pbr",
            L"pbrvertex.hlsl", "main", vsDefines,
            L"pbrpixel.hlsl", "main", psDefines);
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::Gizmo()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = CreateShader(
            "gizmo",
            L"gizmo.hlsl", "VSMain", nullptr,
            L"gizmo.hlsl", "PSMain", nullptr);
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::Grid()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = CreateShader(
            "grid",
            L"grid.hlsl", "VSMain", nullptr,
            L"grid.hlsl", "PSMain", nullptr);
    }
    return s_shader;
}

} // namespace framedata