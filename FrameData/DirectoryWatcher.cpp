#include "DirectoryWatcher.h"
#include <Windows.h>
#include <thread>
#include <memory>

namespace framedata
{
class DirectoryWatcherImpl
{
    std::filesystem::path m_path;
    HANDLE m_hDir = NULL;
    std::thread m_thread;
    DirectoryWatcher::OnFileFunc m_onFile;
    bool m_isEnd = false;

public:
    DirectoryWatcherImpl(const std::filesystem::path &path,
                         const DirectoryWatcher::OnFileFunc &callback)
        : m_path(path), m_onFile(callback)
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
                m_onFile(std::wstring(p->FileName, p->FileName + p->FileNameLength / 2), p->Action);
                // switch (p->Action)
                // {
                // case FILE_ACTION_ADDED:
                //     // wprintf(L"FILE_ACTION_ADDED: %s\n", filename);
                //     break;

                // case FILE_ACTION_REMOVED:
                //     // wprintf(L"FILE_ACTION_REMOVED: %s\n", filename);
                //     break;

                // case FILE_ACTION_MODIFIED:
                //     // wprintf(L"FILE_ACTION_MODIFIED: %s\n", filename);
                //     break;

                // case FILE_ACTION_RENAMED_OLD_NAME:
                //     // wprintf(L"FILE_ACTION_RENAMED_OLD_NAME: %s\n", filename);
                //     break;

                // case FILE_ACTION_RENAMED_NEW_NAME:
                //     // wprintf(L"FILE_ACTION_RENAMED_NEW_NAME: %s\n", filename);
                //     break;

                // default:
                //     // wprintf(L"Unknown File Action: %s\n", filename);
                //     break;
                // }

                if (!p->NextEntryOffset)
                {
                    break;
                }
            }
        }

        auto a = 0;
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

    std::filesystem::path getPath(const std::wstring &shaderName) const
    {
        auto path = m_path;
        return path.append(shaderName);
    }
};

DirectoryWatcher::DirectoryWatcher(const std::filesystem::path &path, const OnFileFunc &callback)
    : m_impl(new DirectoryWatcherImpl(path, callback))
{
}

DirectoryWatcher::~DirectoryWatcher()
{
    delete m_impl;
}

std::filesystem::path DirectoryWatcher::FullPath(const std::wstring &relativeName) const
{
    return m_impl->getPath(relativeName);
}

} // namespace framedata
