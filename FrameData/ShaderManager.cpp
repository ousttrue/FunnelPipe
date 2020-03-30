#include "ShaderManager.h"
#include "DirectoryWatcher.h"
#include <thread>
#include <functional>
#include <fstream>
#include <windows.h>

namespace framedata
{

static std::string ReadAllText(const std::filesystem::path &path)
{
    std::string result;
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (ifs)
    {
        auto pos = ifs.tellg();
        result.resize(pos);
        ifs.seekg(0, std::ios::beg);
        ifs.read((char *)result.data(), pos);
    }
    return result;
}

static std::wstring multi_to_wide_winapi(std::string const &src)
{
    auto const dest_size = ::MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, nullptr, 0U);
    std::vector<wchar_t> dest(dest_size, L'\0');
    if (::MultiByteToWideChar(CP_ACP, 0U, src.data(), -1, dest.data(), (int)dest.size()) == 0)
    {
        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
    }
    dest.resize(std::char_traits<wchar_t>::length(dest.data()));
    dest.shrink_to_fit();
    return std::wstring(dest.begin(), dest.end());
}


ShaderManager::ShaderManager()
{
}

ShaderManager::~ShaderManager()
{
    stop();
}

ShaderManager &ShaderManager::Instance()
{
    static ShaderManager s_instance;
    return s_instance;
}

void ShaderManager::watch(std::filesystem::path &path)
{
    stop();
    m_watcher = new DirectoryWatcher(path, std::bind(&ShaderManager::onFile, this, std::placeholders::_1, std::placeholders::_2));
}

void ShaderManager::stop()
{
    if (m_watcher)
    {
        delete m_watcher;
        m_watcher = nullptr;
    }
}

// default
ShaderWatcherPtr ShaderManager::get(const std::string &shaderName)
{
    auto fileName = multi_to_wide_winapi(shaderName + ".hlsl");
    auto found = m_shaderMap.find(fileName);
    if (found != m_shaderMap.end())
    {
        return found->second;
    }

    auto shader = std::make_shared<ShaderWatcher>(shaderName);
    auto source = ReadAllText(m_watcher->FullPath(fileName));
    shader->source(source);

    {
        std::lock_guard<std::mutex> scoped(m_mutex);
        m_shaderMap.insert(std::make_pair(fileName, shader));
    }

    return shader;
}

void ShaderManager::onFile(const std::wstring &fileName, int action)
{
    if (action == FILE_ACTION_MODIFIED)
    {
        std::lock_guard<std::mutex> scoped(m_mutex);
        auto found = m_shaderMap.find(fileName);
        if (found != m_shaderMap.end())
        {
            auto source = ReadAllText(m_watcher->FullPath(fileName));
            found->second->source(source);
        }
    }
}

} // namespace framedata
