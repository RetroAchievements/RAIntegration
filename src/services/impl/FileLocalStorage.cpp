#include "FileLocalStorage.hh"

#include "ra_fwd.h"
#include "RA_Json.h"
#include "RA_Log.h"
#include "RA_StringUtils.h"

#include "services\IClock.hh"
#include "services\IFileSystem.hh"
#include "services\ServiceLocator.hh"

namespace ra {
namespace services {
namespace impl {

_CONSTANT_VAR RA_DIR_BASE = L"RACache\\";
_CONSTANT_VAR RA_DIR_BADGE = L"RACache\\Badge\\";
_CONSTANT_VAR RA_DIR_DATA = L"RACache\\Data\\";
_CONSTANT_VAR RA_DIR_USERPIC = L"RACache\\UserPic\\";
_CONSTANT_VAR RA_DIR_BOOKMARKS = L"RACache\\Bookmarks\\";

static void PrepareDirectory(const ra::services::IFileSystem& pFileSystem, const std::wstring& sDirectory, bool bExpireOldFiles)
{
    if (!pFileSystem.DirectoryExists(sDirectory))
    {
        pFileSystem.CreateDirectory(sDirectory);
    }
    else if (bExpireOldFiles)
    {
        std::vector<std::wstring> vFiles;
        if (pFileSystem.GetFilesInDirectory(sDirectory, vFiles) > 0)
        {
            const auto& pClock = ServiceLocator::Get<IClock>();
            const auto tExpire = pClock.Now() - std::chrono::hours(24 * 30); // 30 days
            std::wstring sPath;

            for (const auto& sFile : vFiles)
            {
                sPath = sDirectory;
                sPath += sFile;

                // check to see if the file is older than the threshhold
                if (pFileSystem.GetLastModified(sPath) < tExpire)
                {
                    // if it's not a user file, it can be refetched from the server, delete it
                    if (!ra::StringEndsWith(sFile, L"-User.txt"))
                        pFileSystem.DeleteFile(sPath);
                }
            }
        }
    }
}

FileLocalStorage::FileLocalStorage(IFileSystem& pFileSystem)
    : m_pFileSystem(pFileSystem)
{
    // Ensure all required directories are created
    PrepareDirectory(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_BASE, false);
    PrepareDirectory(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_BADGE, true);
    PrepareDirectory(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_DATA, true);
    PrepareDirectory(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_USERPIC, true);
    PrepareDirectory(pFileSystem, pFileSystem.BaseDirectory() + RA_DIR_BOOKMARKS, false);
}

std::wstring FileLocalStorage::GetPath(StorageItemType nType, const std::wstring& sKey) const
{
    std::wstring sPath = m_pFileSystem.BaseDirectory();
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

        case StorageItemType::SessionStats:
            sPath.append(RA_DIR_BASE);
            sPath.append(sKey);
            sPath.append(L"-history.txt");
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

std::chrono::system_clock::time_point FileLocalStorage::GetLastModified(StorageItemType nType, const std::wstring& sKey)
{
    return m_pFileSystem.GetLastModified(GetPath(nType, sKey));
}

std::unique_ptr<TextReader> FileLocalStorage::ReadText(StorageItemType nType, const std::wstring& sKey)
{
    std::wstring sPath = GetPath(nType, sKey);
    if (m_pFileSystem.GetFileSize(sPath) > 0)
        return m_pFileSystem.OpenTextFile(sPath);

    return std::unique_ptr<TextReader>();
}

std::unique_ptr<TextWriter> FileLocalStorage::WriteText(StorageItemType nType, const std::wstring& sKey)
{
    return m_pFileSystem.CreateTextFile(GetPath(nType, sKey));
}

std::unique_ptr<TextWriter> FileLocalStorage::AppendText(StorageItemType nType, const std::wstring& sKey)
{
    return m_pFileSystem.AppendTextFile(GetPath(nType, sKey));
}

} // namespace impl
} // namespace services
} // namespace ra
