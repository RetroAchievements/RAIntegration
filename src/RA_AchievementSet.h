#ifndef RA_ACHIEVEMENTSET_H
#define RA_ACHIEVEMENTSET_H
#pragma once

#include "RA_Achievement.h" // RA_Condition.h (RA_Defs.h)

#if !defined(RAPIDJSON_DOCUMENT_H_) && defined(RA_UTEST)
#define RAPIDJSON_NOMEMBERITERATORCLASS 1
#include <rapidjson\include\rapidjson\document.h>
#endif // !!defined(RAPIDJSON_DOCUMENT_H_) && defined(RA_UTEST)

//////////////////////////////////////////////////////////////////////////
//	AchievementSet
//////////////////////////////////////////////////////////////////////////

class AchievementSet
{
public:
    using Type = ra::detail::AchievementSetType;

    explicit AchievementSet(_In_ Type nType) noexcept :
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

    _Success_(return != 0)
    BOOL LoadFromFile(_Inout_ ra::GameID nGameID);
    BOOL SaveToFile();

    BOOL DeletePatchFile(ra::GameID nGameID);

    _Success_(return != L"")
    _NODISCARD std::wstring GetAchievementSetFilename(_In_ ra::GameID nGameID) noexcept;

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
    const Type m_nSetType;
    std::vector<Achievement> m_Achievements;
    BOOL m_bProcessingActive;
};


//	Externals:

extern AchievementSet* g_pCoreAchievements;
extern AchievementSet* g_pUnofficialAchievements;
extern AchievementSet* g_pLocalAchievements;

extern AchievementSet* g_pActiveAchievements;
extern AchievementSet::Type g_nActiveAchievementSet;

void RASetAchievementCollection(_In_ AchievementSet::Type Type) noexcept;


#endif // !RA_ACHIEVEMENTSET_H
