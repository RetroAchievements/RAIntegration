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
    wchar_t sBuffer[MAX_PATH];
    GetModuleFileNameW(0, sBuffer, MAX_PATH);
    PathRemoveFileSpecW(sBuffer);
    m_sBaseDirectory = sBuffer;
    if (m_sBaseDirectory.back() != '\\')
        m_sBaseDirectory.push_back('\\');
}

bool WindowsFileSystem::DirectoryExists(const std::wstring& sDirectory) const
{
    DWORD nAttrib = GetFileAttributesW(sDirectory.c_str());
    return (nAttrib != INVALID_FILE_ATTRIBUTES) && (nAttrib & FILE_ATTRIBUTE_DIRECTORY);
}

bool WindowsFileSystem::CreateDirectory(const std::wstring& sDirectory) const
{
    return static_cast<bool>(CreateDirectoryW(sDirectory.c_str(), nullptr));
}

bool WindowsFileSystem::DeleteFile(const std::wstring& sPath) const
{
    return static_cast<bool>(DeleteFileW(sPath.c_str()));
}

bool WindowsFileSystem::MoveFile(const std::wstring& sOldPath, const std::wstring& sNewPath) const
{
    return static_cast<bool>(MoveFileW(sOldPath.c_str(), sNewPath.c_str()));
}

int64_t WindowsFileSystem::GetFileSize(const std::wstring& sPath) const
{
    WIN32_FILE_ATTRIBUTE_DATA fadFile;
    if (!GetFileAttributesExW(sPath.c_str(), GET_FILEEX_INFO_LEVELS::GetFileExInfoStandard, &fadFile))
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

    LARGE_INTEGER nSize{ fadFile.nFileSizeLow, ra::narrow_cast<LONG>(fadFile.nFileSizeHigh) };
    return nSize.QuadPart;
}

std::unique_ptr<TextReader> WindowsFileSystem::OpenTextFile(const std::wstring& sPath) const
{
    auto pReader = std::make_unique<FileTextReader>(sPath);
    if (!pReader->GetFStream().is_open())
    {
        RA_LOG("Failed to open \"%s\": %d", ra::Narrow(sPath).c_str(), errno);
        return std::unique_ptr<TextReader>();
    }

    return std::unique_ptr<TextReader>(pReader.release());
}

std::unique_ptr<TextWriter> WindowsFileSystem::CreateTextFile(const std::wstring& sPath) const
{
    auto pWriter = std::make_unique<FileTextWriter>(sPath);
    if (!pWriter->GetFStream().is_open())
    {
        RA_LOG("Failed to create \"%s\": %d", ra::Narrow(sPath).c_str(), errno);
        return std::unique_ptr<TextWriter>();
    }

    return std::unique_ptr<TextWriter>(pWriter.release());
}

std::unique_ptr<TextWriter> WindowsFileSystem::AppendTextFile(const std::wstring& sPath) const
{
    auto pWriter = std::make_unique<FileTextWriter>(sPath, std::ios::app);
    if (!pWriter->GetFStream().is_open())
    {
        if (ra::services::ServiceLocator::Exists<ra::services::ILogger>())
        {
            RA_LOG("Failed to open \"%s\" for append: %d", ra::Narrow(sPath).c_str(), errno);
        }

        return std::unique_ptr<TextWriter>();
    }

    return std::unique_ptr<TextWriter>(pWriter.release());
}

} // namespace impl
} // namespace services
} // namespace ra
