#ifndef _ACHIEVEMENT_H_
#define _ACHIEVEMENT_H_

#include <WTypes.h>
#include <vector>
#include "RA_Condition.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace
{
	extern enum AchievementType
	{
		AT_CORE,
		AT_UNOFFICIAL,
		AT_USER,
		AT__MAX
	};

	extern const char* LockedBadge;
	extern const char* LockedBadgeFile;

};

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////////
//	Achievement
//////////////////////////////////////////////////////////////////////////
enum Achievement_DirtyFlags
{
	Dirty_Title		= 1<<0,
	Dirty_Desc		= 1<<1,
	Dirty_Points	= 1<<2,
	Dirty_Author	= 1<<3,
	Dirty_ID		= 1<<4,
	Dirty_Badge		= 1<<5,
	Dirty_Conditions= 1<<6,
	Dirty_Votes		= 1<<7,
	Dirty_Description= 1<<8,

	Dirty__All		= (unsigned int)(-1)
};

class Achievement
{
public:
	Achievement();

public:
	void Clear();

	BOOL Test();

	size_t AddCondition( size_t nConditionGroup, const Condition& pNewCond );
	BOOL RemoveCondition( size_t nConditionGroup, unsigned int nConditionID );
	void RemoveAllConditions( size_t nConditionGroup );

	inline BOOL Active() const				{ return m_bActive; }
	inline BOOL Modified() const			{ return m_bModified; }
	inline unsigned int ID() const			{ return m_nAchievementID; }
	inline const char* Title() const		{ return m_sTitle; }
	inline const char* Description() const	{ return m_sDescription; }
	inline unsigned int Points() const		{ return m_nPointValue; }
	inline const char* Author() const		{ return m_sAuthor; }
	inline time_t CreatedDate() const		{ return m_nTimestampCreated; }
	inline time_t ModifiedDate() const		{ return m_nTimestampModified; }
	inline unsigned short Upvotes() const	{ return m_nUpvotes; }
	inline unsigned short Downvotes() const	{ return m_nDownvotes; }
	inline const char* Progress() const		{ return m_sProgress; }
	inline const char* ProgressMax() const	{ return m_sProgressMax; }
	inline const char* ProgressFmt() const	{ return m_sProgressFmt; }

	void AddConditionGroup();
	void RemoveConditionGroup();

	inline unsigned int NumConditionGroups() const					{ return m_vConditions.size(); }
	inline unsigned int NumConditions( size_t nGroup ) const		{ return m_vConditions[nGroup].Count(); }

	inline HBITMAP BadgeImage() const				{ return m_hBadgeImage; }
	inline HBITMAP BadgeImageLocked() const			{ return m_hBadgeImageLocked; }
	inline const char* BadgeImageFilename() const	{ return m_sBadgeImageFilename; }
	

	void Set( const Achievement& rRHS );
	void SetActive( BOOL bActive );
	void SetModified( BOOL bModified );
	void SetID( unsigned int nID );
	void SetAuthor( const char* sAuthor );
	void SetTitle( const char* sTitle );
	void SetDescription( const char* sDescription );
	void SetPoints( unsigned int nPoints );
	void SetCreatedDate( time_t nTimeCreated );
	void SetModifiedDate( time_t nTimeModified );
	void SetProgressIndicator( const char* sProgress );
	void SetProgressIndicatorMax( const char* sProgressMax );
	void SetProgressIndicatorFormat( const char* sProgressFmt );
	void SetUpvotes( unsigned short nVal );
	void SetDownvotes( unsigned short nVal );
	void SetBadgeImage( const char* sFilename );

	void ClearBadgeImage();

	Condition& GetCondition( size_t nCondGroup, unsigned int i )	{ return m_vConditions[nCondGroup].GetAt( i ); }

	int CreateMemString( char* pStrOut, const int nNumChars );


	void Reset();
	//int StoreDynamicVar( char* pVarName, CompVariable nVar );

	void UpdateProgress();
	float ProgressGetNextStep( char* sFormat, float fLastKnownProgress );
	float ParseProgressExpression( char* pExp );

	//	Returns the new char* offset after parsing.
	char* ParseLine( char* buffer );

	//	Used for rendering updates when editing achievements. Usually always false.
	unsigned int GetDirtyFlags() const			{ return m_nDirtyFlags; }
	BOOL IsDirty() const						{ return (m_nDirtyFlags!=0); }
	void SetDirtyFlag( unsigned int nFlags )	{ m_nDirtyFlags |= nFlags; }
	void ClearDirtyFlag()						{ m_nDirtyFlags = 0; }

private:
	unsigned int m_nAchievementID;
	std::vector<ConditionSet> m_vConditions;

	char m_sTitle[256];
	char m_sDescription[256];
	char m_sAuthor[256];
	char m_sBadgeImageFilename[16];
	unsigned int m_nPointValue;
	BOOL m_bActive;
	BOOL m_bModified;

	//	Progress:
	BOOL m_bProgressEnabled;	//	on/off
	char m_sProgress[256];		//	How to calculate the progress so far (syntactical)
	char m_sProgressMax[256];	//	Upper limit of the progress (syntactical? value?)
	char m_sProgressFmt[50];	//	Format of the progress to be shown (currency? step?)
	float m_fProgressLastShown;	//	The last shown progress

	unsigned int m_nDirtyFlags;	//	Use for rendering when editing.

	time_t m_nTimestampCreated;
	time_t m_nTimestampModified;
	unsigned short m_nUpvotes;
	unsigned short m_nDownvotes;

	//DynamicVariable m_nDynamicVars[5];
	int m_nNumDynamicVars;

	int m_nInternalArrayOffset;
	HBITMAP m_hBadgeImage;
	HBITMAP m_hBadgeImageLocked;
};

//////////////////////////////////////////////////////////////////////////
//	AchievementSet
//////////////////////////////////////////////////////////////////////////

class AchievementSet
{
public:
	AchievementSet(unsigned int nType)
	{
		m_nType = nType;
		m_nNumAchievements = 0;
		m_nGameID = 0;
		m_bProcessingActive = true;
		m_sPreferredGameTitle[0] = '\0';
	}

public:
	static BOOL DeletePatchFile( unsigned int nAchievementSet, unsigned int nGameID );
	static void GetAchievementsFilename( unsigned int nAchievementSet, unsigned int nGameID, char* pCharOut, int nNumChars );

public:
	void Init();
	void Clear();
	void Test();

	//	Get Achievement at offset
	Achievement& GetAchievement( unsigned int nIter )		{ return m_Achievements[nIter]; }

	//	Add a new achievement to the list, and return a reference to it.
	Achievement& AddAchievement();

	//	Take a copy of the achievement at nIter, add it and return a reference to it.
	Achievement& Clone( unsigned int nIter );

	//	Find achievement with ID, or NULL if it can't be found.
	Achievement* Find( unsigned int nID );

	BOOL RemoveAchievement( unsigned int nIter );

	BOOL Save();
	BOOL Load( const unsigned int nGameID );

	void SaveProgress( const char* sRomName );
	void LoadProgress( const char* sRomName );

	BOOL Unlock( unsigned int nAchievementID );

	unsigned int NumActive();

	void SetPaused( BOOL bPauseState );

	const char* GameTitle();
	void SetGameTitle( const char* pStrIn );

	BOOL HasUnsavedChanges();
	BOOL IsCurrentAchievementSetSelected();

	void SetGameID( unsigned int nGameID )		{ m_nGameID = nGameID; }

	inline unsigned int GameID() const			{ return m_nGameID; }
	inline unsigned int Count() const			{ return m_nNumAchievements; }

public:
	Achievement m_Achievements[256];
	unsigned int m_nNumAchievements;
	BOOL m_bProcessingActive;
	char m_sPreferredGameTitle[64];
	unsigned int m_nGameID;				//	Should be fetched from DB query
	unsigned int m_nType;				//	One of AchievementType::
};

#ifdef __cplusplus
extern "C" {
#endif

	//	Externals:

extern AchievementSet* CoreAchievements;
extern AchievementSet* UnofficialAchievements;
extern AchievementSet* LocalAchievements;

extern AchievementType g_nActiveAchievementSet;
extern AchievementSet* g_pActiveAchievements;

extern void SetAchievementCollection( enum AchievementType Type );

#ifdef __cplusplus
}
#endif

struct _iobuf;
typedef struct _iobuf FILE;

#endif // _ACHIEVEMENT_H_