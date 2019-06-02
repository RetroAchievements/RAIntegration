#ifndef RA_SERVICES_ACHIEVEMENT_RUNTIME_HH
#define RA_SERVICES_ACHIEVEMENT_RUNTIME_HH
#pragma once

#include "RA_AchievementSet.h"

#include "services\TextReader.hh"

#include <string>

#include <rcheevos\include\rcheevos.h>

namespace ra {
namespace services {

class AchievementRuntime
{
public:
    AchievementRuntime() noexcept = default;
    virtual ~AchievementRuntime() noexcept = default;
    AchievementRuntime(const AchievementRuntime&) noexcept = delete;
    AchievementRuntime& operator=(const AchievementRuntime&) noexcept = delete;
    AchievementRuntime(AchievementRuntime&&) noexcept = delete;
    AchievementRuntime& operator=(AchievementRuntime&&) noexcept = delete;

    /// <summary>
    /// Adds an achievement to the processing queue.
    /// </summary>
    void ActivateAchievement(unsigned int nId, rc_trigger_t* pTrigger) noexcept;

    /// <summary>
    /// Adds an achievement to the processing queue and enables the AchievementReset change type for it.
    /// </summary>
    void MonitorAchievementReset(unsigned int nId, rc_trigger_t* pTrigger) noexcept;

    /// <summary>
    /// Removes an achievement from the processing queue.
    /// </summary>
    void DeactivateAchievement(unsigned int nId) noexcept
    {
        RemoveEntry(m_vQueuedAchievements, nId);
        RemoveEntry(m_vActiveAchievements, nId);
        RemoveEntry(m_vActiveAchievementsMonitorReset, nId);
    }

    /// <summary>
    /// Adds a leaderboard to the processing queue.
    /// </summary>
    void ActivateLeaderboard(unsigned int nId, rc_lboard_t* pLeaderboard) noexcept
    {
        GSL_SUPPRESS_F6 AddEntry(m_vActiveLeaderboards, nId, pLeaderboard);
    }

    /// <summary>
    /// Removes a leaderboard from the processing queue.
    /// </summary>
    void DeactivateLeaderboard(unsigned int nId) noexcept
    {
        RemoveEntry(m_vActiveLeaderboards, nId);
    }

    enum class ChangeType
    {
        None = 0,
        AchievementTriggered,
        AchievementReset,
        LeaderboardStarted,
        LeaderboardCanceled,
        LeaderboardUpdated,
        LeaderboardTriggered,
    };

    struct Change
    {
        ChangeType nType;
        unsigned int nId;
        unsigned int nValue;
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
    bool LoadProgress(const char* sLoadStateFilename) const;

    /// <summary>
    /// Writes HitCount data for active achievements to a save state file.
    /// </summary>
    /// <param name="sLoadStateFilename">The name of the save state file.</param>
    void SaveProgress(const char* sSaveStateFilename) const;

    /// <summary>
    /// Gets whether achievement processing is temporarily suspended.
    /// </summary>
    bool IsPaused() const noexcept { return m_bPaused; }

    /// <summary>
    /// Sets whether achievement processing should be temporarily suspended.
    /// </summary>
    void SetPaused(bool bValue) noexcept { m_bPaused = bValue; }

    /// <summary>
    /// Resets any active achievements and disables them until their triggers are false.
    /// </summary>
    void ResetActiveAchievements();

protected:
    struct ActiveAchievement
    {
        ActiveAchievement(rc_trigger_t* pTrigger, unsigned int nId) noexcept : pTrigger(pTrigger), nId(nId) {}

        rc_trigger_t* pTrigger;
        unsigned int nId;
    };

    struct QueuedAchievement : ActiveAchievement
    {
        QueuedAchievement(rc_trigger_t* pTrigger, unsigned int nId) noexcept : ActiveAchievement(pTrigger, nId) {}

        bool bPauseOnReset = false;
    };

    struct ActiveLeaderboard
    {
        ActiveLeaderboard(rc_lboard_t* pLeaderboard, unsigned int nId) noexcept : pLeaderboard(pLeaderboard), nId(nId) {}

        rc_lboard_t* pLeaderboard;
        unsigned int nId;
        unsigned int nValue = 0;
    };

    template<class TCollection, class TData>
    static void AddEntry(std::vector<TCollection>& vEntries, unsigned int nId, TData* pData)
    {
        Expects(pData != nullptr);

        for (const auto& pAchievement : vEntries)
        {
            if (pAchievement.nId == nId)
                return;
        }

        vEntries.emplace_back(pData, nId);
    }

    template<class T>
    GSL_SUPPRESS_F6
    static void RemoveEntry(std::vector<T>& vEntries, unsigned int nId) noexcept
    {
        for (auto pIter = vEntries.begin(); pIter != vEntries.end(); ++pIter)
        {
            if (pIter->nId == nId)
            {
                vEntries.erase(pIter);
                break;
            }
        }
    }

    std::vector<QueuedAchievement> m_vQueuedAchievements;
    std::vector<ActiveAchievement> m_vActiveAchievements;
    std::vector<ActiveAchievement> m_vActiveAchievementsMonitorReset;

    std::vector<ActiveLeaderboard> m_vActiveLeaderboards;

    bool m_bPaused = false;

private:
    bool LoadProgressV1(const std::string& sProgress, std::set<unsigned int>& vProcessedAchievementIds) const;
    bool LoadProgressV2(ra::services::TextReader& pFile, std::set<unsigned int>& vProcessedAchievementIds) const;
};

} // namespace services
} // namespace ra

#endif // !RA_SERVICES_ACHIEVEMENT_RUNTIME_HH
