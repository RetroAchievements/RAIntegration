#ifndef RA_DATA_GAMECONTEXT_HH
#define RA_DATA_GAMECONTEXT_HH
#pragma once

#include "RA_AchievementSet.h"
#include "RA_Leaderboard.h"

#include <string>

namespace ra {
namespace data {

class GameContext
{
public:
    GSL_SUPPRESS_F6 GameContext() = default;
    virtual ~GameContext() noexcept = default;
    GameContext(const GameContext&) noexcept = delete;
    GameContext& operator=(const GameContext&) noexcept = delete;
    GameContext(GameContext&&) noexcept = delete;
    GameContext& operator=(GameContext&&) noexcept = delete;

    enum class Mode
    {
        Normal,
        CompatibilityTest,
    };

    /// <summary>
    /// Loads the data for the specified game id.
    /// </summary>
    virtual void LoadGame(unsigned int nGameId, Mode nMode = Mode::Normal);

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
    /// Gets the current game play mode.
    /// </summary>
    Mode GetMode() const noexcept { return m_nMode; }

    /// <summary>
    /// Sets the game play mode.
    /// </summary>
    void SetMode(Mode nMode) noexcept { m_nMode = nMode; }

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
    Achievement* FindAchievement(ra::AchievementID nAchievementId) const noexcept
    {
        for (auto& pAchievement : m_vAchievements)
        {
            if (pAchievement->ID() == nAchievementId)
                return pAchievement.get();
        }

        return nullptr;
    }

    Achievement& NewAchievement(AchievementSet::Type nType);

    bool RemoveAchievement(ra::AchievementID nAchievementId);

    /// <summary>
    /// Shows the popup for earning an achievement and notifies the server if legitimate.
    /// </summary>
    void AwardAchievement(ra::AchievementID nAchievementId) const;

    /// <summary>
    /// Finds the leaderboard associated to the specified unique identifier.
    /// </summary>
    /// <returns>Pointer to leaderboard, <c>nullptr</c> if not found.</returns>
    RA_Leaderboard* FindLeaderboard(ra::LeaderboardID nLeaderboardId) const noexcept
    {
        for (auto& pLeaderboard : m_vLeaderboards)
        {
            if (pLeaderboard->ID() == nLeaderboardId)
                return pLeaderboard.get();
        }

        return nullptr;
    }

    void DeactivateLeaderboards() noexcept;
    void ActivateLeaderboards();

    size_t LeaderboardCount() const noexcept { return m_vLeaderboards.size(); }

    /// <summary>
    /// Enumerates the leaderboard collection.
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known leaderboard. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateLeaderboards(std::function<bool(const RA_Leaderboard&)> callback) const
    {
        for (auto& pLeaderboard : m_vLeaderboards)
        {
            if (!callback(*pLeaderboard))
                break;
        }
    }

    /// <summary>
    /// Submit a new score for the specified leaderboard.
    /// </summary>
    void SubmitLeaderboardEntry(ra::LeaderboardID nLeaderboardId, unsigned int nScore) const;

    /// <summary>
    /// Updates the set of unlocked achievements from the server.
    /// </summary>
    void RefreshUnlocks() { RefreshUnlocks(false, 0); }

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
    bool ReloadAchievement(ra::AchievementID nAchievementId);

    /// <summary>
    /// Saves local achievement data to local storage.
    /// </summary>
    bool SaveLocal() const;

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>    
    /// <returns>The note associated to the address, <c>nullptr</c> if no note is associated to the address.</returns>
    const std::wstring* FindCodeNote(ra::ByteAddress nAddress) const
    {
        const auto pIter = m_mCodeNotes.find(nAddress);
        return (pIter != m_mCodeNotes.end()) ? &pIter->second.Note : nullptr;
    }

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>
    /// <returns>
    ///  The note associated to the address or neighboring addresses based on <paramref name="nSize" />.
    ///  Returns an empty string if no note is associated to the address.
    /// </returns>
    std::wstring FindCodeNote(ra::ByteAddress nAddress, MemSize nSize) const;

    /// <summary>
    /// Returns the note associated with the specified address.
    /// </summary>
    /// <param name="nAddress">The address to look up.</param>
    /// <param name="sAuthor">The author associated to the address.</param>
    /// <returns>The note associated to the address, <c>nullptr</c> if no note is associated to the address.</returns>
    const std::wstring* FindCodeNote(ra::ByteAddress nAddress, _Inout_ std::string& sAuthor) const
    {
        const auto pIter = m_mCodeNotes.find(nAddress);
        if (pIter == m_mCodeNotes.end())
            return nullptr;

        sAuthor = pIter->second.Author;
        return &pIter->second.Note;
    }

    /// <summary>
    /// Enumerates the code notes
    /// </summary>
    /// <remarks>
    /// <paramref name="callback" /> is called for each known code note. If it returns <c>false</c> enumeration stops.
    /// </remarks>
    void EnumerateCodeNotes(std::function<bool(ra::ByteAddress nAddress)> callback) const
    {
        for (auto& pCodeNote : m_mCodeNotes)
        {
            if (!callback(pCodeNote.first))
                break;
        }
    }

    /// <summary>
    /// Sets the note to associate with the specified address.
    /// </summary>
    /// <param name="nAddress">The address to set the note for.</param>
    /// <param name="sNote">The new note for the address.</param>
    /// <returns><c>true</c> if the note was updated, </c>false</c> if an error occurred.</returns>
    bool SetCodeNote(ra::ByteAddress nAddress, const std::wstring& sNote);

    /// <summary>
    /// Deletes the note associated with the specified address.
    /// </summary>
    /// <param name="nAddress">The address to delete the note for.</param>
    /// <returns><c>true</c> if the note was deleted, </c>false</c> if an error occurred.</returns>
    bool DeleteCodeNote(ra::ByteAddress nAddress);


    class NotifyTarget
    {
    public:
        NotifyTarget() noexcept = default;
        virtual ~NotifyTarget() noexcept = default;
        NotifyTarget(const NotifyTarget&) noexcept = delete;
        NotifyTarget& operator=(const NotifyTarget&) noexcept = delete;
        NotifyTarget(NotifyTarget&&) noexcept = default;
        NotifyTarget& operator=(NotifyTarget&&) noexcept = default;

        virtual void OnActiveGameChanged() noexcept(false) {}
    };

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept { GSL_SUPPRESS_F6 m_vNotifyTargets.insert(&pTarget); }
    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept { GSL_SUPPRESS_F6 m_vNotifyTargets.erase(&pTarget); }

private:
    using NotifyTargetSet = std::set<NotifyTarget*>;

protected:
    void MergeLocalAchievements();
    bool ReloadAchievement(Achievement& pAchievement);
    void RefreshUnlocks(bool bUnpause, int nPopup);
    void UpdateUnlocks(const std::set<unsigned int>& vUnlockedAchievements, bool bUnpause, int nPopup);
    void LoadRichPresenceScript(const std::string& sRichPresenceScript);
    void RefreshCodeNotes();

    void OnActiveGameChanged();

    unsigned int m_nGameId = 0;
    std::wstring m_sGameTitle;
    std::string m_sGameHash;
    Mode m_nMode{};

    ra::AchievementID m_nNextLocalId = 0;
    static const ra::AchievementID FirstLocalId = 111000001;

    void* m_pRichPresence = nullptr;                                   // rc_richpresence_t
    std::shared_ptr<std::vector<unsigned char>> m_pRichPresenceBuffer; // buffer for rc_richpresence_t

    std::vector<std::unique_ptr<Achievement>> m_vAchievements;
    std::vector<std::unique_ptr<RA_Leaderboard>> m_vLeaderboards;

    struct CodeNote
    {
        std::string Author;
        std::wstring Note;
        unsigned int Bytes;
    };
    std::map<ra::ByteAddress, CodeNote> m_mCodeNotes;

private:
    /// <summary>
    /// A collection of pointers to other objects. These are not allocated object and do not need to be free'd. It's
    /// impossible to create a set of <c>NotifyTarget</c> references.
    /// </summary>
    NotifyTargetSet m_vNotifyTargets;
};

} // namespace data
} // namespace ra

#endif // !RA_DATA_GAMECONTEXT_HH
