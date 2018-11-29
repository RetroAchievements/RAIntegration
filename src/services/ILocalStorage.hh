#ifndef RA_SERVICES_ILOCALSTORAGE_HH
#define RA_SERVICES_ILOCALSTORAGE_HH
#pragma once

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
    SessionStats,
};

class ILocalStorage
{
public:
    virtual ~ILocalStorage() noexcept = default;
    ILocalStorage(const ILocalStorage&) noexcept = delete;
    ILocalStorage& operator=(const ILocalStorage&) noexcept = delete;
    ILocalStorage(ILocalStorage&&) noexcept = delete;
    ILocalStorage& operator=(ILocalStorage&&) noexcept = delete;
    
    /// <summary>
    /// Gets the last time the stored data was modified.
    /// </summary>
    /// <returns>Time the stored data was last modified, <c>0</c> if it doesn't exist.</returns>
    virtual std::chrono::system_clock::time_point GetLastModified(StorageItemType nType, const std::wstring& sKey) = 0;

    /// <summary>
    ///   Begins reading stored data for the specified <paramref name="nType" /> and <paramref name="sKey" />.
    /// </summary>
    /// <returns><see cref="TextReader" /> for reading the data, <c>nullptr</c> if not found.</returns>
    virtual std::unique_ptr<TextReader> ReadText(StorageItemType nType, const std::wstring& sKey) = 0;       

    /// <summary>
    ///   Begins writing stored data for the specified <paramref name="nType" /> and <paramref name="sKey" />.
    /// </summary>
    /// <returns>
    ///   <see cref="TextWriter" /> for writing the data, <c>nullptr</c> if the data cannot be written.
    /// </returns>
    virtual std::unique_ptr<TextWriter> WriteText(StorageItemType nType, const std::wstring& sKey) = 0;

    /// <summary>
    ///   Begins appending stored data for the specified <paramref name="nType" /> and <paramref name="sKey" />.
    /// </summary>
    /// <returns>
    ///   <see cref="TextWriter" /> for writing the data, <c>nullptr</c> if the data cannot be written.
    /// </returns>
    virtual std::unique_ptr<TextWriter> AppendText(StorageItemType nType, const std::wstring& sKey) = 0;

protected:
    ILocalStorage() noexcept = default;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ILOCALSTORAGE_HH
