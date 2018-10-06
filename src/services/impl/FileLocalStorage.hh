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
    FileLocalStorage(IFileSystem& pFileSystem);

    std::unique_ptr<TextReader> ReadText(StorageItemType nType, const std::wstring& sKey);

    std::unique_ptr<TextWriter> WriteText(StorageItemType nType, const std::wstring& sKey);

    std::wstring GetPath(StorageItemType nType, const std::wstring& sKey) const;

private:
    IFileSystem& pFileSystem;
};

} // namespace impl
} // namespace services
} // namespace ra

#endif // !RA_SERVICES_FILELOCALSTORAGE_HH
