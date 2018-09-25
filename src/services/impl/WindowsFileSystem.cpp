#include "WindowsFileSystem.hh"

#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "services\impl\FileTextReader.hh"
#include "services\impl\FileTextWriter.hh"

#include <fstream>

#define WIN32_LEAN_AND_MEAN
struct IUnknown;
#include <Windows.h>
#include <Shlwapi.h>
#undef CreateDirectory

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

    RA_LOG("BaseDirectory: %s", ra::Narrow(m_sBaseDirectory).c_str());
}

bool WindowsFileSystem::DirectoryExists(const std::wstring& sDirectory) const
{
    DWORD nAttrib = GetFileAttributesW(sDirectory.c_str());
    return (nAttrib != INVALID_FILE_ATTRIBUTES);
}

bool WindowsFileSystem::CreateDirectory(const std::wstring& sDirectory) const
{
    return static_cast<bool>(CreateDirectoryW(sDirectory.c_str(), nullptr));
}

std::unique_ptr<TextReader> WindowsFileSystem::OpenTextFile(const std::wstring& sPath) const
{
    auto pReader = std::make_unique<FileTextReader>(sPath);
    if (!pReader->GetFStream().is_open())
        return std::unique_ptr<TextReader>();

    return std::unique_ptr<TextReader>(pReader.release());
}

std::unique_ptr<TextWriter> WindowsFileSystem::CreateTextFile(const std::wstring& sPath) const
{
    auto pWriter = std::make_unique<FileTextWriter>(sPath);
    if (!pWriter->GetFStream().is_open())
        return std::unique_ptr<TextWriter>();

    return std::unique_ptr<TextWriter>(pWriter.release());
}

} // namespace impl
} // namespace services
} // namespace ra
