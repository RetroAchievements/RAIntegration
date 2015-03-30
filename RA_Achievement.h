#pragma once

#include <WTypes.h>
#include <vector>
#include "RA_Condition.h"
#include "RA_Defs.h"


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
	Achievement( AchievementSetType nType );

public:
	void Clear();
	BOOL Test();

	BOOL IsCoreAchievement() const		{ return m_nSetType == AchievementSetCore; }

	size_t AddCondition( size_t nConditionGroup, const Condition& pNewCond );
	BOOL RemoveCondition( size_t nConditionGroup, unsigned int nConditionID );
	void RemoveAllConditions( size_t nConditionGroup );
	
	void Set( const Achievement& rRHS );

	inline BOOL Active() const											{ return m_bActive; }
	void SetActive( BOOL bActive );

	inline BOOL Modified() const										{ return m_bModified; }
	void SetModified( BOOL bModified );
	
	void SetID( AchievementID nID );
	inline AchievementID ID() const										{ return m_nAchievementID; }

	inline const std::string& Title() const								{ return m_sTitle; }
	void SetTitle( const std::string& sTitle )							{ m_sTitle = sTitle; }
	inline const std::string& Description() const						{ return m_sDescription; }
	void SetDescription( const std::string& sDescription )				{ m_sDescription = sDescription; }
	inline const std::string& Author() const							{ return m_sAuthor; }
	void SetAuthor( const std::string& sAuthor )						{ m_sAuthor = sAuthor; }
	inline unsigned int Points() const									{ return m_nPointValue; }
	void SetPoints( unsigned int nPoints )								{ m_nPointValue = nPoints; }

	inline time_t CreatedDate() const									{ return m_nTimestampCreated; }
	void SetCreatedDate( time_t nTimeCreated )							{ m_nTimestampCreated = nTimeCreated; }
	inline time_t ModifiedDate() const									{ return m_nTimestampModified; }
	void SetModifiedDate( time_t nTimeModified )						{ m_nTimestampModified = nTimeModified; }
	
	inline const std::string& Progress() const							{ return m_sProgress; }
	void SetProgressIndicator( const std::string& sProgress )			{ m_sProgress = sProgress; }
	inline const std::string& ProgressMax() const						{ return m_sProgressMax; }
	void SetProgressIndicatorMax( const std::string& sProgressMax )		{ m_sProgressMax = sProgressMax; }
	inline const std::string& ProgressFmt() const						{ return m_sProgressFmt; }
	void SetProgressIndicatorFormat( const std::string& sProgressFmt )	{ m_sProgressFmt = sProgressFmt; }
	

	void AddConditionGroup();
	void RemoveConditionGroup();

	inline size_t NumConditionGroups() const						{ return m_vConditions.size(); }
	inline size_t NumConditions( size_t nGroup ) const				{ return m_vConditions[ nGroup ].Count(); }

	inline HBITMAP BadgeImage() const								{ return m_hBadgeImage; }
	inline HBITMAP BadgeImageLocked() const							{ return m_hBadgeImageLocked; }
	inline const std::string& BadgeImageURI() const					{ return m_sBadgeImageURI; }
	
	void SetBadgeImage( const std::string& sFilename );
	void ClearBadgeImage();

	Condition& GetCondition( size_t nCondGroup, size_t i )			{ return m_vConditions[ nCondGroup ].GetAt( i ); }
	
	std::string CreateMemString() const;

	void Reset();

	void UpdateProgress();
	float ProgressGetNextStep( char* sFormat, float fLastKnownProgress );
	float ParseProgressExpression( char* pExp );

	//	Returns the new char* offset after parsing.
	char* ParseLine( char* buffer );
	
	//	Parse from json element
	void Parse( const Value& element );
	
	char* ParseMemString( char* sMem );

	//	Used for rendering updates when editing achievements. Usually always false.
	unsigned int GetDirtyFlags() const			{ return m_nDirtyFlags; }
	BOOL IsDirty() const						{ return (m_nDirtyFlags!=0); }
	void SetDirtyFlag( unsigned int nFlags )	{ m_nDirtyFlags |= nFlags; }
	void ClearDirtyFlag()						{ m_nDirtyFlags = 0; }

private:
	/*const*/ AchievementSetType m_nSetType;

	AchievementID m_nAchievementID;
	std::vector<ConditionSet> m_vConditions;

	std::string m_sTitle;
	std::string m_sDescription;
	std::string m_sAuthor;
	std::string m_sBadgeImageURI;

	unsigned int m_nPointValue;
	BOOL m_bActive;
	BOOL m_bModified;

	//	Progress:
	BOOL m_bProgressEnabled;	//	on/off

	std::string m_sProgress;	//	How to calculate the progress so far (syntactical)
	std::string m_sProgressMax;	//	Upper limit of the progress (syntactical? value?)
	std::string m_sProgressFmt;	//	Format of the progress to be shown (currency? step?)

	float m_fProgressLastShown;	//	The last shown progress

	unsigned int m_nDirtyFlags;	//	Use for rendering when editing.

	time_t m_nTimestampCreated;
	time_t m_nTimestampModified;

	HBITMAP m_hBadgeImage;
	HBITMAP m_hBadgeImageLocked;
};


//////////////////////////////////////////////////////////////////////////
//	AchievementSet
//////////////////////////////////////////////////////////////////////////

class AchievementSet
{
public:
	AchievementSet( AchievementSetType nType ) :
		m_nSetType( nType ),
		m_nGameID( 0 ),
		m_bProcessingActive( true )
	{
	}

public:
	static BOOL DeletePatchFile( AchievementSetType nSet, GameID nGameID );
	static std::string GetAchievementSetFilename( GameID nGameID );
	static BOOL FetchFromWebBlocking( GameID nGameID );
	static BOOL LoadFromFile( GameID nGameID );
	static BOOL SaveToFile();

public:
	void Init();
	void Clear();
	void Test();

	BOOL Serialize( FileStream& Stream );

	//	Get Achievement at offset
	Achievement& GetAchievement( size_t nIter )		{ return m_Achievements[ nIter ]; }

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
	
	BOOL ProcessingActive() const					{ return m_bProcessingActive; }
	void SetPaused( BOOL bIsPaused )				{ m_bProcessingActive = !bIsPaused; }

	const std::string& GameTitle() const			{ return m_sPreferredGameTitle; }
	void SetGameTitle( const std::string& str )		{ m_sPreferredGameTitle = str; }
	
	inline GameID GetGameID() const					{ return m_nGameID; }
	void SetGameID( GameID nGameID )				{ m_nGameID = nGameID; }

	BOOL HasUnsavedChanges();
	BOOL IsCurrentAchievementSetSelected() const;

	inline size_t NumAchievements() const		{ return m_Achievements.size(); }

private:
	const AchievementSetType m_nSetType;
	std::vector<Achievement> m_Achievements;

	std::string m_sPreferredGameTitle;
	GameID m_nGameID;

	BOOL m_bProcessingActive;
};


//	Externals:

extern AchievementSet* CoreAchievements;
extern AchievementSet* UnofficialAchievements;
extern AchievementSet* LocalAchievements;
extern AchievementSet* g_pActiveAchievements;

extern AchievementSetType g_nActiveAchievementSet;
	
extern void SetAchievementCollection( enum AchievementSetType Type );
