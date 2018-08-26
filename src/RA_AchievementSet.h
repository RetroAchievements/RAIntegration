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
    explicit AchievementSet(ra::AchievementSetType nType) noexcept :
        m_nSetType{ nType }
    {
        Clear();
    }

public:
    static bool FetchFromWebBlocking(ra::GameID nGameID);
    static void OnRequestUnlocks(const Document& doc);

public:
    void Clear() noexcept;
    void Test();
    void Reset();

    bool Serialize(FileStream& Stream);
    bool LoadFromFile(ra::GameID nGameID);
    bool SaveToFile();

    bool DeletePatchFile(ra::GameID nGameID);

    std::string GetAchievementSetFilename(ra::GameID nGameID);

    //	Get Achievement at offset
    Achievement& GetAchievement(size_t nIter) { return m_Achievements[nIter]; }
    inline size_t NumAchievements() const { return m_Achievements.size(); }

    // Get Points Total
    inline unsigned int PointTotal() noexcept
    {
        unsigned int total = 0U;
        for (auto& ach : m_Achievements) { total += ach.Points(); }
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

    _Success_(return != false)
    _NODISCARD bool RemoveAchievement(_In_ std::ptrdiff_t nIter);

    void SaveProgress(const char* sRomName);
    void LoadProgress(const char* sRomName);

    bool Unlock(ra::AchievementID nAchievementID);

    unsigned int NumActive() const;

    bool ProcessingActive() const { return m_bProcessingActive; }
    void SetPaused(bool bIsPaused) { m_bProcessingActive = !bIsPaused; }

    bool HasUnsavedChanges();

private:
    bool m_bProcessingActive{true};
    ra::AchievementSetType m_nSetType{};
    std::vector<Achievement> m_Achievements;
};


//	Externals:

extern AchievementSet* g_pCoreAchievements;
extern AchievementSet* g_pUnofficialAchievements;
extern AchievementSet* g_pLocalAchievements;

extern AchievementSet* g_pActiveAchievements;
extern ra::AchievementSetType g_nActiveAchievementSet;

void RASetAchievementCollection(_In_ ra::AchievementSetType Type) noexcept;


#endif // !RA_ACHIEVEMENTSET_H
