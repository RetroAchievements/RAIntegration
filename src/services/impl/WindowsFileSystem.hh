#ifndef RA_SERVICES_WIN32_FILESYSTEM_HH
#define RA_SERVICES_WIN32_FILESYSTEM_HH
#pragma once

#include "ra_fwd.h"

#include "services\IFileSystem.hh"

namespace ra {
namespace services {
namespace impl {

#undef CreateDirectory
#undef DeleteFile
#undef MoveFile

class WindowsFileSystem : public IFileSystem
{
public:
    GSL_SUPPRESS_F6 WindowsFileSystem() noexcept;

    const std::wstring& BaseDirectory() const noexcept override { return m_sBaseDirectory; }
    bool DirectoryExists(const std::wstring& sDirectory) const noexcept override;
    bool CreateDirectory(const std::wstring& sDirectory) const noexcept override;
    size_t GetFilesInDirectory(const std::wstring& sDirectory,
                               _Inout_ std::vector<std::wstring>& vResults) const override;
    bool DeleteFile(const std::wstring& sPath) const noexcept override;
    bool MoveFile(const std::wstring& sOldPath, const std::wstring& sNewPath) const noexcept override;
    int64_t GetFileSize(const std::wstring& sPath) const override;
    std::chrono::system_clock::time_point GetLastModified(const std::wstring& sPath) const override;
    std::unique_ptr<TextReader> OpenTextFile(const std::wstring& sPath) const override;
    std::unique_ptr<TextWriter> CreateTextFile(const std::wstring& sPath) const override;
    std::unique_ptr<TextWriter> AppendTextFile(const std::wstring& sPath) const override;

private:
    const std::wstring& MakeAbsolute(std::wstring& sBuffer, const std::wstring& sPath) const noexcept;

    std::wstring m_sBaseDirectory;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_WIN32_FILESYSTEM_HH
