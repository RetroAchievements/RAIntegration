#ifndef RA_ACHIEVEMENTSET_H
#define RA_ACHIEVEMENTSET_H
#pragma once

#include "RA_Achievement.h" // RA_Condition.h (RA_Defs.h)


//////////////////////////////////////////////////////////////////////////
//	AchievementSet
//////////////////////////////////////////////////////////////////////////

class AchievementSet
{
public:
    AchievementSet(AchievementSetType nType) :
        m_nSetType(nType),
        m_bProcessingActive(TRUE)
    {
        Clear();
    }

public:
    static BOOL FetchFromWebBlocking(ra::GameID nGameID);
    static void OnRequestUnlocks(const rapidjson::Document& doc);

public:
    void Clear();
    void Test();
    void Reset();

    BOOL LoadFromFile(ra::GameID nGameID);
    BOOL SaveToFile();

    BOOL DeletePatchFile(ra::GameID nGameID);

    std::string GetAchievementSetFilename(ra::GameID nGameID);

    //	Get Achievement at offset
    Achievement& GetAchievement(size_t nIter) { return m_Achievements[nIter]; }
    inline size_t NumAchievements() const { return m_Achievements.size(); }

    // Get Points Total
    inline unsigned int PointTotal()
    {
        unsigned int total = 0U;
        for (auto& ach : m_Achievements) total += ach.Points();
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

    void SaveProgress(const char* sRomName);
    void LoadProgress(const char* sRomName);

    BOOL Unlock(ra::AchievementID nAchievementID);

    unsigned int NumActive() const;

    BOOL ProcessingActive() const { return m_bProcessingActive; }
    void SetPaused(BOOL bIsPaused) { m_bProcessingActive = !bIsPaused; }

    BOOL HasUnsavedChanges();

private:
    const AchievementSetType m_nSetType;
    std::vector<Achievement> m_Achievements;
    BOOL m_bProcessingActive;
};


//	Externals:

extern AchievementSet* g_pCoreAchievements;
extern AchievementSet* g_pUnofficialAchievements;
extern AchievementSet* g_pLocalAchievements;

extern AchievementSet* g_pActiveAchievements;
extern AchievementSetType g_nActiveAchievementSet;

extern void RASetAchievementCollection(enum AchievementSetType Type);


#endif // !RA_ACHIEVEMENTSET_H
