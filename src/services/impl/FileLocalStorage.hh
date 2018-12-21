#ifndef RA_SERVICES_FILELOCALSTORAGE_HH
#define RA_SERVICES_FILELOCALSTORAGE_HH
#pragma once

#include "services\IFileSystem.hh"
#include "services\ILocalStorage.hh"

namespace ra {
namespace services {
namespace impl {

class FileLocalStorage : public ILocalStorage
{
public:
    explicit FileLocalStorage(IFileSystem& pFileSystem);

    std::chrono::system_clock::time_point GetLastModified(StorageItemType nType, const std::wstring& sKey) override;

    std::unique_ptr<TextReader> ReadText(StorageItemType nType, const std::wstring& sKey) override;
    std::unique_ptr<TextWriter> WriteText(StorageItemType nType, const std::wstring& sKey) override;
    std::unique_ptr<TextWriter> AppendText(StorageItemType nType, const std::wstring& sKey) override;

    std::wstring GetPath(StorageItemType nType, const std::wstring& sKey) const;

private:
    IFileSystem& m_pFileSystem;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FILELOCALSTORAGE_HH
