#ifndef RA_SERVICES_WIN32_FILESYSTEM_HH
#define RA_SERVICES_WIN32_FILESYSTEM_HH
#pragma once

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
    WindowsFileSystem() noexcept;

    const std::wstring& BaseDirectory() const override { return m_sBaseDirectory; }
    bool DirectoryExists(const std::wstring& sDirectory) const override;
    bool CreateDirectory(const std::wstring& sDirectory) const override;
    bool DeleteFile(const std::wstring& sPath) const override;
    bool MoveFile(const std::wstring& sOldPath, const std::wstring& sNewPath) const override;
    int64_t GetFileSize(const std::wstring& sPath) const override;
    std::unique_ptr<TextReader> OpenTextFile(const std::wstring& sPath) const override;
    std::unique_ptr<TextWriter> CreateTextFile(const std::wstring& sPath) const override;
    std::unique_ptr<TextWriter> AppendTextFile(const std::wstring& sPath) const override;

private:

    std::wstring m_sBaseDirectory;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_WIN32_FILESYSTEM_HH
