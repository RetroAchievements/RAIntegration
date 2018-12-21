#ifndef RA_ACHIEVEMENTSET_H
#define RA_ACHIEVEMENTSET_H
#pragma once

#include "RA_Achievement.h" // RA_Condition.h (ra_fwd.h)

//////////////////////////////////////////////////////////////////////////
//	AchievementSet
//////////////////////////////////////////////////////////////////////////

class AchievementSet
{
public:
    enum class Type
    {
        Core = 3,
        Unofficial = 5,
        Local = 0
    };

    void Clear() noexcept;
    void Test();
    void Reset() noexcept;

    bool SaveToFile() const;

    //	Get Achievement at offset
    Achievement& GetAchievement(size_t nIter)
    {
        Expects((nIter >= 0) && (nIter < m_Achievements.size()));
        return *m_Achievements.at(nIter);
    }
    const Achievement& GetAchievement(size_t nIter) const
    {
        Expects((nIter >= 0) && (nIter < m_Achievements.size()));
        return *m_Achievements.at(nIter);
    }

    inline size_t NumAchievements() const noexcept { return m_Achievements.size(); }

    // Get Points Total
    inline unsigned int PointTotal() noexcept
    {
        unsigned int total = 0U;
        for (auto pAchievement : m_Achievements)
            total += pAchievement->Points();
        return total;
    }

    //	Find achievement with ID, or nullptr if it can't be found.
    Achievement* Find(unsigned int nID);

    //	Find index of the given achievement in the array list (useful for LBX lookups)
    size_t GetAchievementIndex(const Achievement& Ach);

    void AddAchievement(const Achievement* pAchievement)
    {
        m_Achievements.push_back(const_cast<Achievement*>(pAchievement));
    }

    bool RemoveAchievement(const Achievement* pAchievement);

    bool ProcessingActive() const noexcept { return m_bProcessingActive; }
    void SetPaused(BOOL bIsPaused) noexcept { m_bProcessingActive = !bIsPaused; }

    bool HasUnsavedChanges();

    // ranged-for
    inline auto begin() noexcept { return m_Achievements.begin(); }
    inline auto begin() const noexcept { return m_Achievements.begin(); }
    inline auto end() noexcept { return m_Achievements.end(); }
    inline auto end() const noexcept { return m_Achievements.end(); }

private:
    // pointers to items in GameContext.m_vAchievements
    std::vector<Achievement*> m_Achievements;
    bool m_bProcessingActive{ TRUE };
};

//	Externals:

extern AchievementSet* g_pCoreAchievements;
extern AchievementSet* g_pUnofficialAchievements;
extern AchievementSet* g_pLocalAchievements;

extern AchievementSet* g_pActiveAchievements;
extern AchievementSet::Type g_nActiveAchievementSet;

void RASetAchievementCollection(_In_ AchievementSet::Type Type);

#endif // !RA_ACHIEVEMENTSET_H
