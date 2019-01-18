#include "WindowsFileSystem.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "services\impl\FileTextReader.hh"
#include "services\impl\FileTextWriter.hh"

#undef DeleteFile
#undef MoveFile
namespace ra {
namespace services {
namespace impl {

WindowsFileSystem::WindowsFileSystem() noexcept
{
    // determine the home directory from the executable's path
    wchar_t sBuffer[MAX_PATH]{};
    GetModuleFileNameW(nullptr, sBuffer, MAX_PATH);
    PathRemoveFileSpecW(sBuffer);
    m_sBaseDirectory = sBuffer;
    if (!m_sBaseDirectory.empty())
    {
        if (m_sBaseDirectory.back() != L'\\')
            m_sBaseDirectory.push_back(L'\\');
    }
}

GSL_SUPPRESS_F6
const std::wstring& WindowsFileSystem::MakeAbsolute(std::wstring& sBuffer, const std::wstring& sPath) const noexcept
{
    if (!PathIsRelativeW(sPath.c_str()))
        return sPath;

    sBuffer.reserve(m_sBaseDirectory.length() + sPath.length());
    sBuffer.assign(m_sBaseDirectory);
    sBuffer.append(sPath);
    return sBuffer;
}

bool WindowsFileSystem::DirectoryExists(const std::wstring& sDirectory) const noexcept
{
    std::wstring sBuffer;
    const auto& sAbsolutePath = MakeAbsolute(sBuffer, sDirectory);
    const DWORD nAttrib = GetFileAttributesW(sAbsolutePath.c_str());
    return (nAttrib != INVALID_FILE_ATTRIBUTES) && (nAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

bool WindowsFileSystem::CreateDirectory(const std::wstring& sDirectory) const noexcept
{
    std::wstring sBuffer;
    const auto& sAbsolutePath = MakeAbsolute(sBuffer, sDirectory);
    return (CreateDirectoryW(sAbsolutePath.c_str(), nullptr) != 0);
}

size_t WindowsFileSystem::GetFilesInDirectory(const std::wstring& sDirectory, _Inout_ std::vector<std::wstring>& vResults) const
{
    std::wstring sBuffer;
    std::wstring sSearchString = MakeAbsolute(sBuffer, sDirectory);
    sSearchString += L"\\*";

    WIN32_FIND_DATAW ffdFile;
    HANDLE hFind = FindFirstFileW(sSearchString.c_str(), &ffdFile);
    if (hFind == INVALID_HANDLE_VALUE)
        return 0U;

    const size_t nInitialSize = vResults.size();
    do
    {
        if (!(ffdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            vResults.emplace_back(ffdFile.cFileName);
    } while (FindNextFileW(hFind, &ffdFile) != 0);

    FindClose(hFind);
    return vResults.size() - nInitialSize;
}

bool WindowsFileSystem::DeleteFile(const std::wstring& sPath) const noexcept
{
    std::wstring sBuffer;
    const auto& sAbsolutePath = MakeAbsolute(sBuffer, sPath);
    return (DeleteFileW(sAbsolutePath.c_str()) != 0);
}

bool WindowsFileSystem::MoveFile(const std::wstring& sOldPath, const std::wstring& sNewPath) const noexcept
{
    std::wstring sBufferNew, sBufferOld;
    const auto& sAbsolutePathNew = MakeAbsolute(sBufferNew, sNewPath);
    const auto& sAbsolutePathOld = MakeAbsolute(sBufferOld, sOldPath);
    return (MoveFileW(sAbsolutePathOld.c_str(), sAbsolutePathNew.c_str()) != 0);
}

int64_t WindowsFileSystem::GetFileSize(const std::wstring& sPath) const
{
    std::wstring sBuffer;
    const auto& sAbsolutePath = MakeAbsolute(sBuffer, sPath);

    WIN32_FILE_ATTRIBUTE_DATA fadFile;
    if (!GetFileAttributesExW(sAbsolutePath.c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fadFile))
    {
        if (GetLastError() != ERROR_FILE_NOT_FOUND && ra::services::ServiceLocator::Exists<ra::services::ILogger>())
        {
            RA_LOG_ERR("Error %d getting file size: %s", GetLastError(), ra::Narrow(sPath).c_str());
        }

        return -1;
    }

    if (fadFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
        {
            RA_LOG_ERR("File size requested for directory: %s", ra::Narrow(sPath).c_str());
        }

        return -1;
    }

    const LARGE_INTEGER nSize{fadFile.nFileSizeLow, ra::to_signed(fadFile.nFileSizeHigh)};
    return nSize.QuadPart;
}

std::chrono::system_clock::time_point WindowsFileSystem::GetLastModified(const std::wstring& sPath) const
{
    std::wstring sBuffer;
    const auto& sAbsolutePath = MakeAbsolute(sBuffer, sPath);

    WIN32_FILE_ATTRIBUTE_DATA fadFile;
    if (!GetFileAttributesExW(sAbsolutePath.c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fadFile))
    {
        RA_LOG_ERR("Error %d getting last modified for file: %s", GetLastError(), ra::Narrow(sPath).c_str());
        return std::chrono::system_clock::time_point();
    }

    // FILETIME (ftLastWriteTime) is 100-nanosecond intervals since Jan 1 1601. time_t is 1-second intervals since
    // Jan 1 1970. Convert from 100-nanosecond intervals to 1-second intervals, then subtract the number of seconds
    // between the two dates. See https://www.gamedev.net/forums/topic/565693-converting-filetime-to-time_t-on-windows/
    const ULARGE_INTEGER nFileTime{ fadFile.ftLastWriteTime.dwLowDateTime, fadFile.ftLastWriteTime.dwHighDateTime };
    const time_t tFileTime{ra::to_signed((nFileTime.QuadPart / 10000000ULL) - 11644473600ULL)};

    return std::chrono::system_clock::from_time_t(tFileTime);
}

std::unique_ptr<TextReader> WindowsFileSystem::OpenTextFile(const std::wstring& sPath) const
{
    std::wstring sBuffer;
    const auto& sAbsolutePath = MakeAbsolute(sBuffer, sPath);

    auto pReader = std::make_unique<FileTextReader>(sAbsolutePath);
    if (!pReader->GetFStream().is_open())
    {
        RA_LOG("Failed to open \"%s\": %d", ra::Narrow(sPath).c_str(), errno);
        return std::unique_ptr<TextReader>();
    }

    return std::unique_ptr<TextReader>(pReader.release());
}

std::unique_ptr<TextWriter> WindowsFileSystem::CreateTextFile(const std::wstring& sPath) const
{
    std::wstring sBuffer;
    const auto& sAbsolutePath = MakeAbsolute(sBuffer, sPath);

    auto pWriter = std::make_unique<FileTextWriter>(sAbsolutePath);
    if (!pWriter->GetFStream().is_open())
    {
        RA_LOG("Failed to create \"%s\": %d", ra::Narrow(sPath).c_str(), errno);
        return std::unique_ptr<TextWriter>();
    }

    return std::unique_ptr<TextWriter>(pWriter.release());
}

std::unique_ptr<TextWriter> WindowsFileSystem::AppendTextFile(const std::wstring& sPath) const
{
    std::wstring sBuffer;
    const auto& sAbsolutePath = MakeAbsolute(sBuffer, sPath);

    // cannot use std::ios::app, or the SetPosition method doesn't work
    // have to specify std::ios::in or the previous contents are lost
    auto pWriter = std::make_unique<FileTextWriter>(sAbsolutePath, std::ios::ate | std::ios::in | std::ios::out);
    const gsl::not_null<const std::ofstream*> oStream{gsl::make_not_null(&pWriter->GetFStream())};
    if (!oStream->is_open())
    {
        // failed to open the file - try creating it
        std::ofstream file(sPath, std::ios::out);
        if (file.is_open())
        {
            // create succeeded, reopen it
            file.close();

            pWriter = std::make_unique<FileTextWriter>(sPath, std::ios::in | std::ios::out);
        }
        else
        {
            // create failed
            if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
            {
                RA_LOG("Failed to open \"%s\" for append: %d", ra::Narrow(sPath).c_str(), errno);
            }

            return std::unique_ptr<TextWriter>();
        }
    }

    return std::unique_ptr<TextWriter>(pWriter.release());
}

} // namespace impl
} // namespace services
} // namespace ra
