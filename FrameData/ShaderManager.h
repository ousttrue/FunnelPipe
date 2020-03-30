#pragma once
#include "ShaderWatcher.h"
#include <unordered_map>
#include <filesystem>
#include <mutex>

namespace framedata
{

class ShaderManager
{
    std::unordered_map<std::wstring, ShaderWatcherPtr> m_shaderMap;
    std::mutex m_mutex;

    // avoid copy
    ShaderManager(const ShaderManager &) = delete;
    ShaderManager &operator=(const ShaderManager &) = delete;

    ShaderManager();
    ~ShaderManager();

public:
    // singleton
    static ShaderManager &Instance();

    // default
    ShaderWatcherPtr Get(const std::string &shaderName);
    ShaderWatcherPtr GetDefault()
    {
        return Get("default");
    }

    void OnFile(const std::wstring &fileName, int action);
};

} // namespace framedata
