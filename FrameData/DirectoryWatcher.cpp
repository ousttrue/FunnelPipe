#include "DirectoryWatcher.h"
#include <Windows.h>
#include <thread>
#include <memory>
#include <fstream>

namespace framedata
{

std::vector<uint8_t> FileWatcher::Copy()
{
    std::scoped_lock<std::mutex> scoped(m_mutex);
    return m_buffer;
}

std::string FileWatcher::String()
{
    std::scoped_lock<std::mutex> scoped(m_mutex);
    return std::string(m_buffer.begin(), m_buffer.end());
}

void FileWatcher::Read()
{
    std::ifstream ifs(m_path, std::ios::binary | std::ios::ate);
    if (ifs)
    {
        auto pos = ifs.tellg();

        std::scoped_lock<std::mutex> scoped(m_mutex);
        m_buffer.resize(pos);
        ifs.seekg(0, std::ios::beg);
        ifs.read((char *)m_buffer.data(), pos);
    }
}

class DirectoryWatcherImpl
{
    std::filesystem::path m_path;
    HANDLE m_hDir = NULL;
    std::thread m_thread;
    std::mutex m_mutex;
    bool m_isEnd = false;

    std::unordered_map<std::wstring, FileWatcherPtr> m_watcherMap;

public:
    DirectoryWatcherImpl(const std::filesystem::path &path)
        : m_path(path)
    {
        auto hDir = CreateFileW((LPCWSTR)path.u16string().c_str(),
                                FILE_LIST_DIRECTORY,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);
        if (hDir == INVALID_HANDLE_VALUE)
        {
            throw "CreateFile failed.";
        }
        m_hDir = hDir;

        m_thread = std::thread(std::bind(&DirectoryWatcherImpl::OnFile, this));
    }

    ~DirectoryWatcherImpl()
    {
        m_isEnd = true;

        // create dummy file for ReadDirectoryChangesW blocking
        {
            auto path = m_path;
            path.append("tmp.tmp");
            auto hFile = CreateFileW((LPCWSTR)path.u16string().c_str(), // name of the write
                                     GENERIC_WRITE,                     // open for writing
                                     0,                                 // do not share
                                     NULL,                              // default security
                                     CREATE_NEW,                        // create new file only
                                     FILE_ATTRIBUTE_NORMAL,             // normal file
                                     NULL);                             // no attr. template
            int a = 0;
            DWORD write;
            WriteFile(hFile, &a, 4, &write, NULL);
            CloseHandle(hFile);
            DeleteFileW((LPCWSTR)path.u16string().c_str());
        }

        m_thread.join();

        if (m_hDir)
        {
            CloseHandle(m_hDir);
            m_hDir = nullptr;
        }
    }

    FileWatcherPtr Get(const std::wstring &file)
    {
        FileWatcherPtr watcher;
        {
            std::scoped_lock<std::mutex> scoped(m_mutex);

            auto found = m_watcherMap.find(file);
            if (found != m_watcherMap.end())
            {
                return found->second;
            }

            auto path = m_path;
            path.append(file);
            watcher = std::make_shared<FileWatcher>(path);
            m_watcherMap.insert(std::make_pair(file, watcher));
        }

        watcher->Read();
        return watcher;
    }

    void UpdateFile(const std::wstring &file, int action)
    {
        if (action != FILE_ACTION_MODIFIED)
        {
            return;
        }

        std::scoped_lock<std::mutex> scoped(m_mutex);
        auto found = m_watcherMap.find(file);
        if (found == m_watcherMap.end())
        {
            return;
        }
        found->second->Read();
    }

    void OnFile()
    {
        while (true)
        {
            if (m_isEnd)
            {
                break;
            }
            uint8_t buffer[1024] = {};
            DWORD dwBytesReturned;
            auto bRet = ReadDirectoryChangesW(m_hDir,
                                              buffer,
                                              sizeof(buffer),
                                              TRUE,
                                              FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SIZE,
                                              &dwBytesReturned,
                                              NULL,
                                              NULL);
            if (!bRet)
            {
                throw "ReadDirectoryChangesW failed.";
            }

            if (dwBytesReturned == 0)
            {
                throw;
            }

            FILE_NOTIFY_INFORMATION *p = nullptr;
            for (DWORD i = 0; i < dwBytesReturned; i += p->NextEntryOffset)
            {
                p = (FILE_NOTIFY_INFORMATION *)&buffer[i];
                UpdateFile(std::wstring(p->FileName, p->FileName + p->FileNameLength / 2), p->Action);

                if (!p->NextEntryOffset)
                {
                    break;
                }
            }
        }
    }

    // std::filesystem::path getPath(const std::wstring &shaderName) const
    // {
    //     auto path = m_path;
    //     return path.append(shaderName);
    // }
};

DirectoryWatcher::DirectoryWatcher()
{
}

DirectoryWatcher::~DirectoryWatcher()
{
    Stop();
}

DirectoryWatcher &DirectoryWatcher::Instance()
{
    static DirectoryWatcher s_instance;
    return s_instance;
}

void DirectoryWatcher::Watch(const std::filesystem::path &path)
{
    Stop();
    m_impl = new DirectoryWatcherImpl(path);
}

void DirectoryWatcher::Stop()
{
    if (m_impl)
    {
        delete m_impl;
        m_impl = nullptr;
    }
}

FileWatcherPtr DirectoryWatcher::Get(const std::wstring &relativePath)
{
    return m_impl->Get(relativePath);
}

} // namespace framedata
