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
}

ShaderManager &ShaderManager::Instance()
{
    static ShaderManager s_instance;
    return s_instance;
}

// default
ShaderWatcherPtr ShaderManager::GetSource(const std::string &shaderName, bool isInclude)
{
    auto fileName = multi_to_wide_winapi(shaderName);
    auto found = m_watcherMap.find(fileName);
    if (found != m_watcherMap.end())
    {
        return found->second;
    }

    auto watcher = std::make_shared<ShaderWatcher>(shaderName, isInclude);
    auto source = ReadAllText(DirectoryWatcher::Instance().FullPath(fileName));
    watcher->source(source);

    {
        std::lock_guard<std::mutex> scoped(m_mutex);
        m_watcherMap.insert(std::make_pair(fileName, watcher));
    }

    return watcher;
}

void ShaderManager::OnFile(const std::wstring &fileName, int action)
{
    if (action == FILE_ACTION_MODIFIED)
    {
        std::lock_guard<std::mutex> scoped(m_mutex);
        auto found = m_watcherMap.find(fileName);
        if (found != m_watcherMap.end())
        {
            auto source = ReadAllText(DirectoryWatcher::Instance().FullPath(fileName));
            found->second->source(source);
        }
    }
}

} // namespace framedata
