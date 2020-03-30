#include "ShaderManager.h"
#include "DirectoryWatcher.h"

namespace framedata
{

ShaderManager &ShaderManager::Instance()
{
    static ShaderManager s_instance;
    return s_instance;
}

static ShaderPassPtr CreateShader(const std::string &name,
                                  const std::wstring &vs, const std::string &vsEntryPoint,
                                  const std::wstring &ps, const std::string &psEntryPoint,
                                  const D3D_SHADER_MACRO *pDefine)
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
        if (!shader->VS->Compile(source, vsEntryPoint, pDefine))
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
        if (!shader->PS->Compile(source, psEntryPoint, pDefine))
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
        s_shader = CreateShader("unlit",
                                L"gltf_unlit.hlsl", "VSMain",
                                L"gltf_unlit.hlsl", "PSMain",
                                nullptr);
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::GltfPBR()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        D3D_SHADER_MACRO defines[] = {
            {"NORMALS", ""},
            {"UV", ""},
            {nullptr, nullptr},
        };
        s_shader = CreateShader("pbr",
                                L"pbrvertex.hlsl", "main",
                                L"pbrpixel.hlsl", "main",
                                defines);
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::Gizmo()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = CreateShader("gizmo",
                                L"gizmo.hlsl", "VSMain",
                                L"gizmo.hlsl", "PSMain",
                                nullptr);
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::Grid()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = CreateShader("grid",
                                L"grid.hlsl", "VSMain",
                                L"grid.hlsl", "PSMain",
                                nullptr);
    }
    return s_shader;
}

} // namespace framedata