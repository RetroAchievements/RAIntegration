#ifndef RA_SERVICES_ACHIEVEMENT_RUNTIME_HH
#define RA_SERVICES_ACHIEVEMENT_RUNTIME_HH
#pragma once

#include "ra_fwd.h"

#include "data\Types.hh"
#include "data\context\EmulatorContext.hh"

#include "services\TextReader.hh"

#include <string>

#include <rcheevos\include\rcheevos.h>

namespace ra {
namespace services {

class AchievementRuntime : protected ra::data::context::EmulatorContext::NotifyTarget
{
public:
    GSL_SUPPRESS_F6 AchievementRuntime() = default;
    virtual ~AchievementRuntime()
    {
        ResetRuntime();
    }

    AchievementRuntime(const AchievementRuntime&) noexcept = delete;
    AchievementRuntime& operator=(const AchievementRuntime&) noexcept = delete;
    AchievementRuntime(AchievementRuntime&&) noexcept = delete;
    AchievementRuntime& operator=(AchievementRuntime&&) noexcept = delete;

    /// <summary>
    /// Clears all active achievements/leaderboards/rich presence from the runtime.
    /// </summary>
    void ResetRuntime() noexcept;

    /// <summary>
    /// Adds an achievement to the processing queue.
    /// </summary>
    int ActivateAchievement(ra::AchievementID nId, const std::string& sTrigger);

    /// <summary>
    /// Removes an achievement from the processing queue.
    /// </summary>
    void DeactivateAchievement(ra::AchievementID nId);

    /// <summary>
    /// Updates any references using the provided ID to a new value
    /// </summary>
    void UpdateAchievementId(ra::AchievementID nOldId, ra::AchievementID nNewId);

    /// <summary>
    /// Gets the raw trigger for the achievement (if active)
    /// </summary>
    rc_trigger_t* GetAchievementTrigger(ra::AchievementID nId) noexcept
    {
        if (!m_bInitialized)
            return nullptr;

        return rc_runtime_get_achievement(&m_pRuntime, nId);
    }

    /// <summary>
    /// Gets the raw trigger for the achievement (if active)
    /// </summary>
    const rc_trigger_t* GetAchievementTrigger(ra::AchievementID nId) const noexcept
    {
        if (!m_bInitialized)
            return nullptr;

        return rc_runtime_get_achievement(&m_pRuntime, nId);
    }

    /// <summary>
    /// Gets the raw trigger for the achievement (if ever active)
    /// </summary>
    rc_trigger_t* GetAchievementTrigger(ra::AchievementID nId, const std::string& sTrigger);

    /// <summary>
    /// Gets the raw trigger for the achievement (if active)
    /// </summary>
    rc_trigger_t* DetachAchievementTrigger(ra::AchievementID nId);

    /// <summary>
    /// Removes an achievement from the processing queue.
    /// </summary>
    void ReleaseAchievementTrigger(ra::AchievementID nId, _In_ rc_trigger_t* pTrigger);

    /// <summary>
    /// Adds a leaderboard to the processing queue.
    /// </summary>
    int ActivateLeaderboard(unsigned int nId, const std::string& sDefinition);

    /// <summary>
    /// Removes a leaderboard from the processing queue.
    /// </summary>
    void DeactivateLeaderboard(unsigned int nId) noexcept
    {
        if (m_bInitialized)
            rc_runtime_deactivate_lboard(&m_pRuntime, nId);
    }

    /// <summary>
    /// Specifies the rich presence to process each frame.
    /// </summary>
    /// <remarks>Only updates the memrefs each frame (for deltas), the script is not processed here.</remarks>
    void ActivateRichPresence(const std::string& sScript);

    /// <summary>
    /// Gets whether or not the loaded game has a rich presence script.
    /// </summary>
    bool HasRichPresence() const noexcept
    {
        if (!m_bInitialized)
            return false;

        return (m_pRuntime.richpresence != nullptr && m_pRuntime.richpresence->richpresence != nullptr);
    }

    /// <summary>
    /// Gets the current rich presence display string.
    /// </summary>
    std::wstring GetRichPresenceDisplayString() const;


    enum class ChangeType
    {
        None = 0,
        AchievementTriggered,
        AchievementReset,
        AchievementPaused,
        LeaderboardStarted,
        LeaderboardCanceled,
        LeaderboardUpdated,
        LeaderboardTriggered,
        AchievementDisabled,
        LeaderboardDisabled,
        AchievementPrimed,
        AchievementActivated,
    };

    struct Change
    {
        ChangeType nType;
        unsigned int nId;
        int nValue;
    };

    /// <summary>
    /// Processes all active achievements for the current frame.
    /// </summary>
    virtual void Process(_Inout_ std::vector<Change>& changes);

    /// <summary>
    /// Loads HitCount data for active achievements from a save state file.
    /// </summary>
    /// <param name="sLoadStateFilename">The name of the save state file.</param>
    /// <returns><c>true</c> if the achievement HitCounts were modified, <c>false</c> if not.</returns>
    bool LoadProgressFromFile(const char* sLoadStateFilename);

    /// <summary>
    /// Loads HitCount data for active achievements from a buffer.
    /// </summary>
    /// <param name="pBuffer">The buffer to read from.</param>
    /// <returns><c>true</c> if the achievement HitCounts were modified, <c>false</c> if not.</returns>
    bool LoadProgressFromBuffer(const char* pBuffer);

    /// <summary>
    /// Writes HitCount data for active achievements to a save state file.
    /// </summary>
    /// <param name="sLoadStateFilename">The name of the save state file.</param>
    void SaveProgressToFile(const char* sSaveStateFilename) const;

    /// <summary>
    /// Writes HitCount data for active achievements to a buffer.
    /// </summary>
    /// <param name="pBuffer">The buffer to write to.</param>
    /// <param name="nBufferSize">The size of the buffer to write to.</param>
    /// <returns>
    /// The numberof bytes required to capture the HitCount data (may be larger than 
    /// nBufferSize - in which case the caller should allocate the specified amount
    /// and call again.
    /// </returns>
    int SaveProgressToBuffer(char* pBuffer, int nBufferSize) const;

    /// <summary>
    /// Gets whether achievement processing is temporarily suspended.
    /// </summary>
    bool IsPaused() const noexcept { return m_bPaused; }

    /// <summary>
    /// Sets whether achievement processing should be temporarily suspended.
    /// </summary>
    void SetPaused(bool bValue);

    /// <summary>
    /// Resets any active achievements and disables them until their triggers are false.
    /// </summary>
    void ResetActiveAchievements() noexcept
    {
        if (m_bInitialized)
            rc_runtime_reset(&m_pRuntime);
    }

    void InvalidateAddress(ra::ByteAddress nAddress) noexcept;

    size_t DetectUnsupportedAchievements();

protected:
    bool m_bPaused = false;
    rc_runtime_t m_pRuntime{};
    mutable std::mutex m_pMutex;

    // EmulatorContext::NotifyTarget
    void OnTotalMemorySizeChanged() override;

private:
    bool LoadProgressV1(const std::string& sProgress, std::set<unsigned int>& vProcessedAchievementIds);
    bool LoadProgressV2(ra::services::TextReader& pFile, std::set<unsigned int>& vProcessedAchievementIds);

    void EnsureInitialized() noexcept;

    std::map<unsigned int, std::string> m_vQueuedAchievements;
    int m_nRichPresenceParseResult = RC_OK;
    int m_nRichPresenceErrorLine = 0;
    bool m_bInitialized = false;
};

} // namespace services
} // namespace ra

extern "C" unsigned int rc_peek_callback(unsigned int nAddress, unsigned int nBytes, _UNUSED void* pData);

#endif // !RA_SERVICES_ACHIEVEMENT_RUNTIME_HH
