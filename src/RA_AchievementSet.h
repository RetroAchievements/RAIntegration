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
    AchievementSet() noexcept = default;
#pragma warning(push)
    // I think this is because you are initializing it as a pointer instead of an object
#pragma warning(disable : 4514) // unreferenced inline functions
    AchievementSet(AchievementSetType nType) noexcept :
        AchievementSet{}
    {
        m_nSetType = nType;
        Clear();
    }
#pragma warning(pop)

    ~AchievementSet() noexcept = default;
    AchievementSet(const AchievementSet&) = delete;
    AchievementSet& operator=(const AchievementSet&) = delete;
    AchievementSet(AchievementSet&&) noexcept = default;
    AchievementSet& operator=(AchievementSet&&) noexcept = default;
public:
    static BOOL FetchFromWebBlocking(GameID nGameID);
    static void OnRequestUnlocks(const Document& doc);

public:
    void Clear();
    void Test();

    BOOL Serialize(FileStream& Stream);
    BOOL LoadFromFile(GameID nGameID);
    BOOL SaveToFile();

    BOOL DeletePatchFile(GameID nGameID);

    std::string GetAchievementSetFilename(GameID nGameID);



#pragma warning(push)
#pragma warning(disable : 4514) // unreferenced inline functions
    //	Get Achievement at offset
    Achievement& GetAchievement(size_t nIter) { return m_Achievements[nIter]; }
    inline size_t NumAchievements() const { return m_Achievements.size(); }


    // Get Points Total
    inline unsigned int PointTotal()
    {
        unsigned int total = 0u;
        for (Achievement& ach : m_Achievements) total += ach.Points();
        return total;
    }

    BOOL ProcessingActive() const { return m_bProcessingActive; }
    void SetPaused(BOOL bIsPaused) { m_bProcessingActive = !bIsPaused; }
#pragma warning(pop)

    

    //	Add a new achievement to the list, and return a reference to it.
    Achievement& AddAchievement();

    //	Take a copy of the achievement at nIter, add it and return a reference to it.
    Achievement& Clone(unsigned int nIter);

    //	Find achievement with ID, or nullptr if it can't be found.
    Achievement* Find(AchievementID nID);

    //	Find index of the given achievement in the array list (useful for LBX lookups)
    size_t GetAchievementIndex(const Achievement& Ach);

    BOOL RemoveAchievement(size_t nIter);

    void SaveProgress(const char* sRomName);
    void LoadProgress(const char* sRomName);

    BOOL Unlock(AchievementID nAchievementID);

    unsigned int NumActive() const;

    BOOL HasUnsavedChanges();

private:
    // delete copying
    AchievementSetType m_nSetType ={};
    std::vector<Achievement> m_Achievements;
    BOOL m_bProcessingActive ={};
};


//	Externals:

extern AchievementSet* g_pCoreAchievements;
extern AchievementSet* g_pUnofficialAchievements;
extern AchievementSet* g_pLocalAchievements;

extern AchievementSet* g_pActiveAchievements;
extern AchievementSetType g_nActiveAchievementSet;

extern void RASetAchievementCollection(enum AchievementSetType Type);


#endif // !RA_ACHIEVEMENTSET_H
