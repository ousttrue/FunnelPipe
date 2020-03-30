#pragma once
#include <filesystem>
#include <string>
#include <functional>

namespace framedata
{
class DirectoryWatcher
{
public:
    using OnFileFunc = const std::function<void(const std::wstring &, int)>;

private:
    class DirectoryWatcherImpl *m_impl = nullptr;

    DirectoryWatcher();
    ~DirectoryWatcher();

public:
    // singleton
    static DirectoryWatcher &Instance();

    void Watch(const std::filesystem::path &path, const OnFileFunc &callback);
    void Stop();

    std::filesystem::path FullPath(const std::wstring &relativeName) const;
};

} // namespace framedata
