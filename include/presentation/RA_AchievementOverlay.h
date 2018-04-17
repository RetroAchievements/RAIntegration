#pragma once

#include <WTypes.h>
#include "RA_Achievement.h"
#include "RA_User.h"
#include "RA_Core.h"
#include "RA_Interface.h"

#include <ddraw.h>

enum OverlayPage
{
	OP_ACHIEVEMENTS,
	OP_FRIENDS,
	OP_MESSAGES,
	OP_NEWS,
	OP_LEADERBOARDS,

	OP_ACHIEVEMENT_EXAMINE,
	OP_ACHIEVEMENT_COMPARE,
	OP_FRIEND_EXAMINE,
	OP_FRIEND_ADD,
	OP_LEADERBOARD_EXAMINE,
	OP_MESSAGE_VIEWER,

	NumOverlayPages
};

enum TransitionState
{
	TS_OFF = 0,
	TS_IN,
	TS_HOLD,
	TS_OUT,

	TS__MAX
};

class LeaderboardExamine
{
public:
	void Initialize( const unsigned int nLBIDIn );
	//static void CB_OnReceiveData( void* pRequestObject );
	void OnReceiveData( const Document& doc );
	
public:
	unsigned int m_nLBID;
	//	Refer to RA_Leaderboard entry rank info via g_LeaderboardManager.FindLB(m_nLBID)
	bool m_bHasData;
};
extern LeaderboardExamine g_LBExamine;


class AchievementExamine
{
public:
	class RecentWinnerData
	{
	public:
		RecentWinnerData( const std::string& sUser, const std::string& sWonAt ) :
			m_sUser( sUser ), m_sWonAt( sWonAt )
		{}
		const std::string& User() const		{ return m_sUser; }
		const std::string& WonAt() const	{ return m_sWonAt; }

	private:
		const std::string m_sUser;
		const std::string m_sWonAt;
	};

public:
	AchievementExamine();

public:
	void Initialize( const Achievement* pAchIn );
	void Clear();
	static void CB_OnReceiveData( void* pRequestObject );
	void OnReceiveData( Document& doc );
	
	bool HasData() const											{ return m_bHasData; }
	const std::string& CreatedDate() const							{ return m_CreatedDate; }
	const std::string& ModifiedDate() const							{ return m_LastModifiedDate; }
	size_t NumRecentWinners() const									{ return RecentWinners.size(); }
	const RecentWinnerData& GetRecentWinner( size_t nOffs ) const	{ return RecentWinners.at( nOffs ); }
	
	unsigned int TotalWinners() const								{ return m_nTotalWinners; }
	unsigned int PossibleWinners() const							{ return m_nPossibleWinners; }

private:
	const Achievement* m_pSelectedAchievement;
	std::string m_CreatedDate;
	std::string m_LastModifiedDate;
	
	bool m_bHasData;

	//	Data found:
	unsigned int m_nTotalWinners;
	unsigned int m_nPossibleWinners;

	std::vector<RecentWinnerData> RecentWinners;
};
extern AchievementExamine g_AchExamine;


class AchievementOverlay
{
public:
	AchievementOverlay();
	~AchievementOverlay();

	void Initialize( HINSTANCE hInst );
	void InstallHINSTANCE( HINSTANCE hInst );

	void Activate();
	void Deactivate();

	void Render( HDC hDC, RECT* rcDest ) const;
	BOOL Update( ControllerInput* input, float fDelta, BOOL bFullScreen, BOOL bPaused );

	BOOL IsActive() const	{ return( m_nTransitionState!=TS_OFF ); }

	const int* GetActiveScrollOffset() const;
	const int* GetActiveSelectedItem() const;

	void OnLoad_NewRom();
	void OnUserPicDownloaded( const char* sUsername );

	void DrawAchievementsPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const;
	void DrawAchievementExaminePage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const;
	void DrawMessagesPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const;
	void DrawFriendsPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const;
	void DrawNewsPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const;
	void DrawLeaderboardPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const;
	void DrawLeaderboardExaminePage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const;

	void DrawBar( HDC hDC, int nX, int nY, int nW, int nH, int nMax, int nSel ) const;
	void DrawUserFrame( HDC hDC, RAUser* pUser, int nX, int nY, int nW, int nH ) const;
	void DrawAchievement( HDC hDC, const Achievement* Ach, int nX, int nY, BOOL bSelected, BOOL bCanLock ) const;

	OverlayPage CurrentPage()		{ return m_Pages[ m_nPageStackPointer ]; }
	void AddPage( OverlayPage NewPage );
	BOOL GoBack();

	void SelectNextTopLevelPage( BOOL bPressedRight );
	
	void InitDirectX();
	void ResetDirectX();
	void CloseDirectX();
	void Flip(HWND hWnd);

	void InstallNewsArticlesFromFile();

public:
	struct NewsItem
	{
		unsigned int m_nID;
		std::string m_sTitle;
		std::string m_sPayload;
		time_t m_nPostedAt;
		std::string m_sPostedAt;
		std::string m_sAuthor;
		std::string m_sLink;
		std::string m_sImage;
	};

private:
	int	m_nAchievementsScrollOffset;
	int	m_nFriendsScrollOffset;
	int	m_nMessagesScrollOffset;
	int	m_nNewsScrollOffset;
	int	m_nLeaderboardScrollOffset;

	int	m_nAchievementsSelectedItem;
	int	m_nFriendsSelectedItem;
	int	m_nMessagesSelectedItem;
	int	m_nNewsSelectedItem;
	int	m_nLeaderboardSelectedItem;

	mutable int m_nNumAchievementsBeingRendered;
	mutable int m_nNumFriendsBeingRendered;
	mutable int m_nNumLeaderboardsBeingRendered;

	BOOL					m_bInputLock;	//	Waiting for pad release
	std::vector<NewsItem>	m_LatestNews;
	TransitionState			m_nTransitionState;
	float					m_fTransitionTimer;

	OverlayPage				m_Pages[ 5 ];
	unsigned int			m_nPageStackPointer;

	//HBITMAP m_hLockedBitmap;	//	Cached	
	HBITMAP m_hOverlayBackground;

	LPDIRECTDRAW4 m_lpDD;
	LPDIRECTDRAWSURFACE4 m_lpDDS_Overlay;
};
extern AchievementOverlay g_AchievementOverlay;

//	Exposed to DLL
extern "C"
{
	API extern int _RA_UpdateOverlay( ControllerInput* pInput, float fDTime, bool Full_Screen, bool Paused );
	API extern void _RA_RenderOverlay( HDC hDC, RECT* rcSize );

	API extern void _RA_InitDirectX();
	API extern void _RA_OnPaint( HWND hWnd );
}

extern const COLORREF COL_TEXT;
extern const COLORREF COL_TEXT_HIGHLIGHT;
extern const COLORREF COL_SELECTED;
extern const COLORREF COL_WHITE;
extern const COLORREF COL_BLACK;
extern const COLORREF COL_POPUP;
extern const COLORREF COL_POPUP_BG;
extern const COLORREF COL_POPUP_SHADOW;
