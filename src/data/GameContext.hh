#ifndef RA_DATA_GAMECONTEXT_HH
#define RA_DATA_GAMECONTEXT_HH
#pragma once

#include "RA_RichPresence.h"
#include "RA_AchievementSet.h"

#include <string>

namespace ra {
namespace data {

class GameContext
{
public:
    GameContext() noexcept = default;
    virtual ~GameContext() noexcept = default;
    GameContext(const GameContext&) noexcept = delete;
    GameContext& operator=(const GameContext&) noexcept = delete;
    GameContext(GameContext&&) noexcept = delete;
    GameContext& operator=(GameContext&&) noexcept = delete;

    /// <summary>
    /// Loads the data for the specified game id.
    /// </summary>
    void LoadGame(unsigned int nGameId);

    /// <summary>
    /// Gets the unique identifier of the currently loaded game.
    /// </summary>
    unsigned int GameId() const noexcept { return m_nGameId; }

    /// <summary>
    /// Gets the title of the currently loaded game.
    /// </summary>
    const std::wstring& GameTitle() const noexcept { return m_sGameTitle; }

    /// <summary>
    /// Sets the game title.
    /// </summary>
    void SetGameTitle(const std::wstring& sGameTitle) { m_sGameTitle = sGameTitle; }

    /// <summary>
    /// Gets the hash of the currently loaded game.
    /// </summary>
    const std::string& GameHash() const noexcept { return m_sGameHash; }

    /// <summary>
    /// Sets the game hash.
    /// </summary>
    void SetGameHash(const std::string& sGameHash) { m_sGameHash = sGameHash; }

    /// <summary>
    /// Gets which achievements are active.
    /// </summary>
    virtual AchievementSet::Type ActiveAchievementType() const noexcept
    {
#ifdef RA_UTEST
        return AchievementSet::Type::Core;
#else
        return g_nActiveAchievementSet;
#endif
    }

    /// <summary>
    /// Enumerates the achievement collection.
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known achievement. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateAchievements(std::function<bool(const Achievement&)> callback) const
    {
        for (auto& pAchievement : m_vAchievements)
        {
            if (!callback(*pAchievement))
                break;
        }
    }

    /// <summary>
    /// Finds the achievement associated to the specified unique identifier.
    /// </summary>
    /// <returns>Pointer to achievement, <c>nullptr</c> if not found.</returns>
    virtual Achievement* FindAchievement(unsigned int nAchievementId) const noexcept
    {
        for (auto& pAchievement : m_vAchievements)
        {
            if (pAchievement->ID() == nAchievementId)
                return pAchievement.get();
        }

        return nullptr;
    }

    Achievement& NewAchievement(AchievementSet::Type nType);

    bool RemoveAchievement(unsigned int nAchievementId) noexcept;

    /// <summary>
    /// Shows the popup for earning an achievement and notifies the server if legitimate.
    /// </summary>
    void AwardAchievement(unsigned int nAchievementId) const;

    /// <summary>
    /// Updates the set of unlocked achievements from the server.
    /// </summary>
    void RefreshUnlocks() { RefreshUnlocks(false); }

    /// <summary>
    /// Gets whether or not the loaded game has a rich presence script.
    /// </summary>
    virtual bool HasRichPresence() const noexcept;
    
    /// <summary>
    /// Gets the current rich presence display string.
    /// </summary>
    virtual std::wstring GetRichPresenceDisplayString() const;
    
    /// <summary>
    /// Reloads the rich presence script.
    /// </summary>
    void ReloadRichPresenceScript();

    /// <summary>
    /// Reloads all achievements of the specified category from local storage.
    /// </summary>
    void ReloadAchievements(int nCategory);

    /// <summary>
    /// Reloads a specific achievement from local storage.
    /// </summary>    
    /// <returns><c>true</c> if the achievement was reloaded, <c>false</c> if it was not found or destroyed.</returns>
    /// <remarks>Destroys the achievement if it does not exist in local storage</remarks>
    bool ReloadAchievement(unsigned int nAchievementId);

    /// <summary>
    /// Saves local achievement data to local storage.
    /// </summary>
    bool SaveLocal() const;

protected:
    void MergeLocalAchievements();
    bool ReloadAchievement(Achievement& pAchievement);
    void RefreshUnlocks(bool bWasPaused);

    unsigned int m_nGameId = 0;
    std::wstring m_sGameTitle;
    std::string m_sGameHash;

    unsigned int m_nNextLocalId = 0;
    static const unsigned int FirstLocalId = 111000001;

    std::unique_ptr<RA_RichPresenceInterpreter> m_pRichPresenceInterpreter;

    std::vector<std::unique_ptr<Achievement>> m_vAchievements;
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_GAMECONTEXT_HH
