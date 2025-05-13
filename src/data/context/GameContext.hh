#ifndef RA_DATA_GAMECONTEXT_HH
#define RA_DATA_GAMECONTEXT_HH
#pragma once

#include "GameAssets.hh"

#include <string>
#include <atomic>

namespace ra { namespace services { class AchievementRuntimeExports; } }

namespace ra {
namespace data {
namespace context {

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
    virtual void LoadGame(unsigned int nGameId, const std::string& sGameHash, Mode nMode = Mode::Normal);

    /// <summary>
    /// Loads the data from the game currently loaded in the AchievementRuntime.
    /// </summary>
    void InitializeFromAchievementRuntime(const std::map<uint32_t, std::string> mAchievementDefinitions,
                                          const std::map<uint32_t, std::string> mLeaderboardDefinitions);

    /// <summary>
    /// Determines whether a game is being loaded.
    /// </summary>
    bool IsGameLoading() const noexcept { return m_nLoadCount != 0; }

    /// <summary>
    /// Gets the unique identifier of the currently loaded game.
    /// </summary>
    unsigned int GameId() const noexcept { return m_nGameId; }

    /// <summary>
    /// Gets the unique identifier of the currently loaded subset game.
    /// </summary>
    /// <remarks>
    /// Will match <see cref="GameId"/> unless an exclusive or specialty subset is loaded.
    /// </remarks>
    unsigned int ActiveGameId() const noexcept { return m_nActiveGameId; }

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
    void SetMode(Mode nMode)
    {
        if (m_nMode != nMode)
        {
            m_nMode = nMode;

            if (m_nGameId != 0)
                OnActiveGameChanged();
        }
    }

    /// <summary>
    /// Determines if the provided game identifier is virtual
    /// </summary>
    /// <remarks>
    /// IDs above 1 billion are incompatible. The 100 millions place indicates how.
    /// Mask off the part above 100 million to get the actual game id.
    /// </remarks>
    static constexpr bool IsVirtualGameId(uint32_t nGameId) noexcept { return nGameId > 1000000000; }

    /// <summary>
    /// Gets the real game identifier from a virtual one
    /// </summary>
    static constexpr uint32_t GetRealGameId(uint32_t nGameId) noexcept { return nGameId % 100000000; }

    enum HashCompatibility
    {
        Compatible = 0,
        Incompatible = 10,
        Untested = 11,
        PatchRequired = 12,
    };

    /// <summary>
    /// Extracts the hash compatibility information from a virtual game identifier
    /// </summary>
    static constexpr HashCompatibility GetHashCompatibility(uint32_t nGameId) noexcept
    {
        return ra::itoe<HashCompatibility>(nGameId / 100000000);
    }

    /// <summary>
    /// Gets the assets for the current game.
    /// </summary>
    GameAssets& Assets() noexcept { return m_vAssets; }
    const GameAssets& Assets() const noexcept { return m_vAssets; }

    class NotifyTarget
    {
    public:
        NotifyTarget() noexcept = default;
        virtual ~NotifyTarget() noexcept = default;
        NotifyTarget(const NotifyTarget&) noexcept = delete;
        NotifyTarget& operator=(const NotifyTarget&) noexcept = delete;
        NotifyTarget(NotifyTarget&&) noexcept = default;
        NotifyTarget& operator=(NotifyTarget&&) noexcept = default;

        virtual void OnBeforeActiveGameChanged() noexcept(false) {}
        virtual void OnActiveGameChanged() noexcept(false) {}
        virtual void OnBeginGameLoad() noexcept(false) {}
        virtual void OnEndGameLoad() noexcept(false) {}
        virtual void OnCodeNoteChanged(ra::ByteAddress, const std::wstring&) noexcept(false) {}
    };

    void AddNotifyTarget(NotifyTarget& pTarget) noexcept { GSL_SUPPRESS_F6 m_vNotifyTargets.insert(&pTarget); }
    void RemoveNotifyTarget(NotifyTarget& pTarget) noexcept { GSL_SUPPRESS_F6 m_vNotifyTargets.erase(&pTarget); }

    void DoFrame();

    enum SubsetType
    {
        Core,
        Bonus,
        Specialty,
        Exclusive,
    };

    class Subset
    {
    public:
        Subset(uint32_t nAchievementSetId, uint32_t nGameId, const std::wstring& sTitle, SubsetType nType) :
            m_sTitle(sTitle), m_nType(nType), m_nAchievementSetId(nAchievementSetId), m_nGameId(nGameId)
        {
        }

        const std::wstring& Title() const noexcept { return m_sTitle; }
        SubsetType Type() const noexcept { return m_nType; }
        uint32_t AchievementSetID() const noexcept { return m_nAchievementSetId; }
        uint32_t GameID() const noexcept { return m_nGameId; }

        // Core assets have SubsetId of 0
        uint32_t ID() const noexcept { return (m_nType == SubsetType::Core) ? 0 : m_nAchievementSetId; }

    private:
        std::wstring m_sTitle;
        SubsetType m_nType;
        uint32_t m_nAchievementSetId;
        uint32_t m_nGameId;
    };
    const std::vector<Subset>& Subsets() const noexcept { return m_vSubsets; }
    void InitializeSubsets(const char* pPatchData, size_t nPatchDataLength);

private:
    using NotifyTargetSet = std::set<NotifyTarget*>;
    void FinishLoadGame(int nResult, const char* sErrorMessage, bool bWasPaused);

    friend class ra::services::AchievementRuntimeExports;
    bool BeginLoadGame(unsigned int nGameId, Mode nMode, bool& bWasPaused);
    void EndLoadGame(int nResult, bool bWasPaused, bool bShowSoftcoreWarning);

protected:
    void OnBeforeActiveGameChanged();
    void OnActiveGameChanged();
    void OnCodeNoteChanged(ra::ByteAddress nAddress, const std::wstring& sNewNote);
    void BeginLoad();
    void EndLoad();

    unsigned int m_nGameId = 0;
    unsigned int m_nActiveGameId = 0;
    std::wstring m_sGameTitle;
    std::string m_sGameHash;
    Mode m_nMode{};

private:
    /// <summary>
    /// A collection of pointers to other objects. These are not allocated object and do not need to be free'd. It's
    /// impossible to create a set of <c>NotifyTarget</c> references.
    /// </summary>
    NotifyTargetSet m_vNotifyTargets;

    GameAssets m_vAssets;
    std::vector<Subset> m_vSubsets;

    std::atomic<int> m_nLoadCount = 0;
    int m_nMasteryPopupId = 0;

    std::mutex m_mLoadMutex;
};

} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_GAMECONTEXT_HH
