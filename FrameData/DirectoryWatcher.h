#pragma once
#include <filesystem>
#include <string>
#include <functional>
#include <mutex>

namespace framedata
{

class FileWatcher
{
    std::filesystem::path m_path;
    std::mutex m_mutex;
    std::vector<uint8_t> m_buffer;

public:
    FileWatcher(std::filesystem::path &path)
        : m_path(path)
    {
    }
    std::vector<uint8_t> Copy();
    std::string String();
    void Read();
};
using FileWatcherPtr = std::shared_ptr<FileWatcher>;

class DirectoryWatcher
{

public:
private:
    class DirectoryWatcherImpl *m_impl = nullptr;

    DirectoryWatcher();
    ~DirectoryWatcher();

public:
    // singleton
    static DirectoryWatcher &Instance();

    void Watch(const std::filesystem::path &path);
    void Stop();

    // std::filesystem::path FullPath(const std::wstring &relativeName) const;
    FileWatcherPtr Get(const std::wstring &relativePath);
};

} // namespace framedata
