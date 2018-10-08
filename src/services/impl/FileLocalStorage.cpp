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

FileLocalStorage::FileLocalStorage(IFileSystem& pFileSystem) noexcept
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
    sPath.reserve(sPath.length() + sKey.length() + 32); // assume max 20 chars for path and 12 chars for suffix

    switch (nType)
    {
        case StorageItemType::GameData:
            sPath.append(RA_DIR_DATA);
            sPath.append(sKey);
            sPath.append(L".json");
            break;

        case StorageItemType::CodeNotes:
            sPath.append(RA_DIR_DATA);
            sPath.append(sKey);
            sPath.append(L"-Notes.json");
            break;

        case StorageItemType::RichPresence:
            sPath.append(RA_DIR_DATA);
            sPath.append(sKey);
            sPath.append(L"-Rich.txt");
            break;

        case StorageItemType::UserAchievements:
            sPath.append(RA_DIR_DATA);
            sPath.append(sKey);
            sPath.append(L"-User.txt");
            break;

        case StorageItemType::Badge:
            sPath.append(RA_DIR_BADGE);
            sPath.append(sKey);
            sPath.append(L".png");
            break;

        case StorageItemType::UserPic:
            sPath.append(RA_DIR_USERPIC);
            sPath.append(sKey);
            sPath.append(L".png");
            break;

        default:
            assert(!"unhandled StorageItemType");
            sPath.append(RA_DIR_DATA);
            sPath.append(sKey);
            sPath.append(L".txt");
            break;
    }

    return sPath;
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
