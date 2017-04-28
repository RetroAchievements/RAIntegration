#pragma once

#include <vector>
#include "RA_Condition.h"
#include "RA_Defs.h"
#include "RA_Achievement.h"


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

	//	Get Achievement at offset
	Achievement& GetAchievement(size_t nIter)	{ return m_Achievements[nIter]; }
	inline size_t NumAchievements() const		{ return m_Achievements.size(); }

	//	Add a new achievement to the list, and return a reference to it.
	Achievement& AddAchievement();

	//	Take a copy of the achievement at nIter, add it and return a reference to it.
	Achievement& Clone(unsigned int nIter);

	//	Find achievement with ID, or NULL if it can't be found.
	Achievement* Find(AchievementID nID);

	//	Find index of the given achievement in the array list (useful for LBX lookups)
	size_t GetAchievementIndex(const Achievement& Ach);

	BOOL RemoveAchievement(size_t nIter);

	void SaveProgress(const char* sRomName);
	void LoadProgress(const char* sRomName);

	BOOL Unlock(AchievementID nAchievementID);

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
	
extern void RASetAchievementCollection( enum AchievementSetType Type );
