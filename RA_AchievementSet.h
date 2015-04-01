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
	AchievementSet( AchievementSetType nType ) :
		m_nSetType( nType ),
		m_bProcessingActive( TRUE )
	{
		Clear();
	}

public:
	static BOOL DeletePatchFile( AchievementSetType nSet, GameID nGameID );
	static std::string GetAchievementSetFilename( GameID nGameID );
	static BOOL FetchFromWebBlocking( GameID nGameID );
	static BOOL LoadFromFile( GameID nGameID );
	static BOOL SaveToFile();
	
	static const std::string& GameTitle()				{ return m_sPreferredGameTitle; }
	static void SetGameTitle( const std::string& str )	{ m_sPreferredGameTitle = str; }
	static inline GameID GetGameID()					{ return m_nGameID; }
	static void SetGameID( GameID nGameID )				{ m_nGameID = nGameID; }

	static void OnRequestUnlocks( const Document& doc );

public:
	void Init();
	void Clear();
	void Test();

	BOOL Serialize( FileStream& Stream );

	//	Get Achievement at offset
	Achievement& GetAchievement( size_t nIter )			{ return m_Achievements[ nIter ]; }
	inline size_t NumAchievements() const				{ return m_Achievements.size(); }

	//	Add a new achievement to the list, and return a reference to it.
	Achievement& AddAchievement();

	//	Take a copy of the achievement at nIter, add it and return a reference to it.
	Achievement& Clone( unsigned int nIter );

	//	Find achievement with ID, or NULL if it can't be found.
	Achievement* Find( AchievementID nID );

	//	Find index of the given achievement in the array list (useful for LBX lookups)
	size_t GetAchievementIndex( const Achievement& Ach );

	BOOL RemoveAchievement( unsigned int nIter );

	void SaveProgress( const char* sRomName );
	void LoadProgress( const char* sRomName );

	BOOL Unlock( unsigned int nAchievementID );

	unsigned int NumActive() const;
	
	BOOL ProcessingActive() const						{ return m_bProcessingActive; }
	void SetPaused( BOOL bIsPaused )					{ m_bProcessingActive = !bIsPaused; }

	BOOL HasUnsavedChanges();

private:
	static std::string m_sPreferredGameTitle;
	static GameID m_nGameID;
	
private:
	const AchievementSetType m_nSetType;
	std::vector<Achievement> m_Achievements;
	BOOL m_bProcessingActive;
};


//	Externals:

extern AchievementSet* CoreAchievements;
extern AchievementSet* UnofficialAchievements;
extern AchievementSet* LocalAchievements;
extern AchievementSet* g_pActiveAchievements;
extern AchievementSetType g_nActiveAchievementSet;
	
extern void RASetAchievementCollection( enum AchievementSetType Type );
