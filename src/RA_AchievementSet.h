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
        Core,
        Unofficial,
        Local
    };

    explicit AchievementSet(_In_ Type eType) noexcept : m_nSetType{eType} { Clear(); }

public:
    static BOOL FetchFromWebBlocking(unsigned int nGameID);
#ifndef RA_UTEST
    static void OnRequestUnlocks(const rapidjson::Document& doc);
#endif // !RA_UTEST

public:
    void Clear() noexcept;
    void Test();
    void Reset() noexcept;

    _Success_(return ) bool LoadFromFile(_Inout_ unsigned int nGameID);
    bool SaveToFile() const;

    //	Get Achievement at offset
    Achievement& GetAchievement(size_t nIter)
    {
        Expects((nIter >= 0) && (nIter < m_Achievements.size()));
        return m_Achievements.at(nIter);
    }
    const Achievement& GetAchievement(size_t nIter) const
    {
        Expects((nIter >= 0) && (nIter < m_Achievements.size()));
        return m_Achievements.at(nIter);
    }
    inline size_t NumAchievements() const noexcept { return m_Achievements.size(); }

    // Get Points Total
    inline unsigned int PointTotal() noexcept
    {
        unsigned int total = 0U;
        for (auto& ach : m_Achievements)
            total += ach.Points();
        return total;
    }

    //	Add a new achievement to the list, and return a reference to it.
    Achievement& AddAchievement();

    //	Take a copy of the achievement at nIter, add it and return a reference to it.
    Achievement& Clone(unsigned int nIter);

    //	Find achievement with ID, or nullptr if it can't be found.
    Achievement* Find(ra::AchievementID nID);

    //	Find index of the given achievement in the array list (useful for LBX lookups)
    size_t GetAchievementIndex(const Achievement& Ach);

    BOOL RemoveAchievement(size_t nIter);

    unsigned int NumActive() const;

    BOOL ProcessingActive() const noexcept { return m_bProcessingActive; }
    void SetPaused(BOOL bIsPaused) noexcept { m_bProcessingActive = !bIsPaused; }

    BOOL HasUnsavedChanges() const;

    // ranged-for
    inline auto begin() noexcept { return m_Achievements.begin(); }
    inline auto begin() const noexcept { return m_Achievements.begin(); }
    inline auto end() noexcept { return m_Achievements.end(); }
    inline auto end() const noexcept { return m_Achievements.end(); }

private:
    const Type m_nSetType{};
    std::vector<Achievement> m_Achievements;
    BOOL m_bProcessingActive{TRUE};
};

//	Externals:

extern AchievementSet* g_pCoreAchievements;
extern AchievementSet* g_pUnofficialAchievements;
extern AchievementSet* g_pLocalAchievements;

extern AchievementSet* g_pActiveAchievements;
extern AchievementSet::Type g_nActiveAchievementSet;

void RASetAchievementCollection(_In_ AchievementSet::Type Type);

#endif // !RA_ACHIEVEMENTSET_H
