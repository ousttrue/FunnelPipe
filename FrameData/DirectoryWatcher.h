#pragma once
#include <filesystem>
#include <string>
#include <functional>

namespace framedata
{
class DirectoryWatcher
{
    class DirectoryWatcherImpl *m_impl = nullptr;

public:
    using OnFileFunc = const std::function<void(const std::wstring &, int)>;
    DirectoryWatcher(const std::filesystem::path &path, const OnFileFunc &callback);
    ~DirectoryWatcher();
    std::filesystem::path FullPath(const std::wstring &relativeName) const;
};

} // namespace framedata
