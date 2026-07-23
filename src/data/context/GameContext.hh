#ifndef RA_DATA_GAMECONTEXT_HH
#define RA_DATA_GAMECONTEXT_HH
#pragma once

#include "context/IGameContext.hh"

#include "data/models/GameAssets.hh"
#include "data/models/LocalBadgesModel.hh"

#include "services/ServiceLocator.hh"

#include <string>
#include <atomic>

struct rc_api_fetch_game_sets_response_t;

namespace ra { namespace services { class AchievementRuntimeExports; } }

namespace ra {
namespace data {
namespace context {

class GameContext : public ra::context::IGameContext
{
public:
    GSL_SUPPRESS_F6 GameContext()
        : m_IGameContextOverride(this)
    {
    }
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
    ra::data::models::GameAssets& Assets() noexcept { return m_vAssets; }
    const ra::data::models::GameAssets& Assets() const noexcept { return m_vAssets; }

    void DoFrame();

    void InitializeSubsets(const rc_api_fetch_game_sets_response_t* game_data_response);
    uint32_t GetGameId(uint32_t nSubsetId) const noexcept;

    ra::data::models::MemoryNotesModel& MemoryNotes() override
    {
        auto* pNotes = m_vAssets.FindMemoryNotes();
        return pNotes ? *pNotes : m_oDefaultNotes;
    }
    const ra::data::models::MemoryNotesModel& MemoryNotes() const override
    {
        auto* pNotes = m_vAssets.FindMemoryNotes();
        return pNotes ? *pNotes : m_oDefaultNotes;
    }

    ra::data::models::LocalBadgesModel& LocalBadges() override
    {
        auto* pLocalBadges = dynamic_cast<ra::data::models::LocalBadgesModel*>(m_vAssets.FindAsset(ra::data::models::AssetType::LocalBadges, 0));
        return pLocalBadges ? *pLocalBadges : m_oDefaultLocalBadges;
    }
    const ra::data::models::LocalBadgesModel& LocalBadges() const override
    {
        auto* pLocalBadges = dynamic_cast<const ra::data::models::LocalBadgesModel*>(m_vAssets.FindAsset(ra::data::models::AssetType::LocalBadges, 0));
        return pLocalBadges ? *pLocalBadges : m_oDefaultLocalBadges;
    }

private:
    void FinishLoadGame(int nResult, const char* sErrorMessage, bool bWasPaused);
    void MigrateSubsetUserFiles();

    friend class ra::services::AchievementRuntimeExports;
    bool BeginLoadGame(unsigned int nGameId, Mode nMode, bool& bWasPaused);
    void EndLoadGame(int nResult, bool bWasPaused, bool bShowSoftcoreWarning);

protected:
    void OnBeforeActiveGameChanged();
    void OnActiveGameChanged();
    void OnMemoryNoteChanged(ra::data::ByteAddress nAddress, const std::wstring& sNewNote);
    void OnMemoryNoteMoved(ra::data::ByteAddress nOldAddress, ra::data::ByteAddress nNewAddress, const std::wstring& sNote);
    void BeginLoad();
    void EndLoad();

    std::string m_sGameHash;
    Mode m_nMode{};

private:
    ra::data::models::GameAssets m_vAssets;

    std::atomic<int> m_nLoadCount = 0;
    int m_nMasteryPopupId = 0;

    std::mutex m_mLoadMutex;

    // use the mock helper to register as both GameContext and IGameContext
    ra::services::ServiceLocator::ServiceOverride<ra::context::IGameContext> m_IGameContextOverride;
    ra::data::models::MemoryNotesModel m_oDefaultNotes;
    ra::data::models::LocalBadgesModel m_oDefaultLocalBadges;
};

} // namespace context
} // namespace data
} // namespace ra

#endif // !RA_DATA_GAMECONTEXT_HH
