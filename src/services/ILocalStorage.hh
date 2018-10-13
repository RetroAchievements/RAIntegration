#ifndef RA_SERVICES_ILOCALSTORAGE_HH
#define RA_SERVICES_ILOCALSTORAGE_HH
#pragma once

#include <string>

#include "TextReader.hh"
#include "TextWriter.hh"

namespace ra {
namespace services {

enum class StorageItemType
{
    None = 0,
    GameData,
    CodeNotes,
    RichPresence,
    UserAchievements,
    Badge,
    UserPic,
};

class ILocalStorage
{
public:
    virtual ~ILocalStorage() noexcept = default;

    /// <summary>
    /// Begins reading stored data for the specified <paramref name="nType"/> and <paramref name="sKey" />.
    /// </summary>
    /// <returns><see cref="TextReader" /> for reading the data, </c>nullptr</c> if not found.</returns>
    virtual std::unique_ptr<TextReader> ReadText(StorageItemType nType, const std::wstring& sKey) = 0;

    /// <summary>
    /// Begins writing stored data for the specified <paramref name="nType"/> and <paramref name="sKey" />.
    /// </summary>
    /// <returns><see cref="TextWriter" /> for writing the data, </c>nullptr</c> if the data cannot be written.</returns>
    virtual std::unique_ptr<TextWriter> WriteText(StorageItemType nType, const std::wstring& sKey) = 0;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ILOCALSTORAGE_HH
