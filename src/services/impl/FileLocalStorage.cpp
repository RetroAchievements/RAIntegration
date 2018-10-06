#include "FileLocalStorage.hh"

#include "ra_fwd.h"
#include "RA_Json.h"
#include "RA_Log.h"

#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

#include <fstream>

namespace ra {
namespace services {
namespace impl {

_CONSTANT_VAR RA_DIR_BASE = L"RACache\\";
_CONSTANT_VAR RA_DIR_BADGE = L"RACache\\Badge\\";
_CONSTANT_VAR RA_DIR_DATA = L"RACache\\Data\\";
_CONSTANT_VAR RA_DIR_USERPIC = L"RACache\\UserPic\\";
_CONSTANT_VAR RA_DIR_BOOKMARKS = L"RACache\\Bookmarks\\";

static void EnsureDirectoryExists(const ra::services::IFileSystem& pFileSystem, const std::wstring& sDirectory)
{
    if (!pFileSystem.DirectoryExists(sDirectory))
        pFileSystem.CreateDirectory(sDirectory);
}

FileLocalStorage::FileLocalStorage(IFileSystem& pFileSystem)
    : pFileSystem(pFileSystem)
{
    // Ensure all required directories are created
    EnsureDirectoryExists(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_BASE);
    EnsureDirectoryExists(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_BADGE);
    EnsureDirectoryExists(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_DATA);
    EnsureDirectoryExists(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_USERPIC);
    EnsureDirectoryExists(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_BOOKMARKS);
}

std::wstring FileLocalStorage::GetPath(StorageItemType nType, const std::wstring& sKey) const
{
    std::wstring sPath = pFileSystem.BaseDirectory();

    switch (nType)
    {
        case StorageItemType::GameData:
            return sPath + RA_DIR_DATA + sKey + L".json";

        case StorageItemType::CodeNotes:
            return sPath + RA_DIR_DATA + sKey + L"-Notes.json";

        case StorageItemType::RichPresence:
            return sPath + RA_DIR_DATA + sKey + L"-Rich.txt";

        case StorageItemType::UserAchievements:
            return sPath + RA_DIR_DATA + sKey + L"-User.txt";

        case StorageItemType::Badge:
            return sPath + RA_DIR_BADGE + sKey + L".png";

        case StorageItemType::UserPic:
            return sPath + RA_DIR_USERPIC + sKey + L".png";

        default:
            assert(!"unhandled StorageItemType");
            return sPath + RA_DIR_DATA + sKey + L".txt";
    }
}

std::unique_ptr<TextReader> FileLocalStorage::ReadText(StorageItemType nType, const std::wstring& sKey)
{
    return pFileSystem.OpenTextFile(GetPath(nType, sKey));
}

std::unique_ptr<TextWriter> FileLocalStorage::WriteText(StorageItemType nType, const std::wstring& sKey)
{
    return pFileSystem.CreateTextFile(GetPath(nType, sKey));
}

} // namespace impl
} // namespace services
} // namespace ra
