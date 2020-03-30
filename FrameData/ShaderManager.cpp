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
                                  const std::wstring &ps, const std::string &psEntryPoint)
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
        shader->VS->Compile(source, vsEntryPoint);
    }
    {
        shader->PS = std::make_shared<framedata::PixelShader>(name + "@ps");
        auto watcher = framedata::DirectoryWatcher::Instance().Get(ps);
        auto source = watcher->String();
        if (source.empty())
        {
            throw;
        }
        shader->PS->Compile(source, psEntryPoint);
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
                                L"gltf_unlit.hlsl", "PSMain");
    }
    return s_shader;
}

ShaderPassPtr ShaderManager::GltfPBR()
{
    static ShaderPassPtr s_shader;
    if (!s_shader)
    {
        s_shader = CreateShader("pbr",
                                L"pbrvertex.hlsl", "main",
                                L"pbrpixel.hlsl", "main");
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
                                L"gizmo.hlsl", "PSMain");
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
                                L"grid.hlsl", "PSMain");
    }
    return s_shader;
}

} // namespace framedata