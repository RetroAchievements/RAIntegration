#if defined (RA_VBA)
#include "stdafx.h"
#endif

#include "RA_AchievementOverlay.h"

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

#include "RA_Interface.h"
#include "RA_Achievement.h"
#include "RA_User.h"
#include "RA_httpthread.h"
#include "RA_resource.h"
#include "RA_ImageFactory.h"
#include "RA_PopupWindows.h"
#include "RA_Core.h"
#include "RA_Leaderboard.h"

//	No emulator-specific code here please!

#define SCROLL_TIMER_START  (1.0f)
#define SCROLL_TIMER_REPEAT (0.4f)

#define PAGE_TRANSITION_IN  (-0.2f)
#define PAGE_TRANSITION_OUT ( 0.2f)


#define NUM_MESSAGES_TO_DRAW (4)

#define FONT_TO_USE "Tahoma"

const char* g_sPageTitles[] = { 
	" Achievements ", 
	" Friends ", 
	" Messages ",
	" News ", 
	" Leaderboards ", 

	" Achievement Info ", 
	" Achievement Compare ",
	" Friend Info ",
	" Friend Add ",
	" Leaderboard Examine ", 
	" Message Viewer "

	};	//	NB. Update enum OverlayPage in header!


HFONT g_hFontTitle;
HFONT g_hFontDesc;
HFONT g_hFontDesc2;
HFONT g_hFontTiny;
HBRUSH g_hBrushBG;
HBRUSH g_hBrushSelectedBG;

AchievementOverlay g_AchievementOverlay;
AchievementExamine g_AchExamine;
LeaderboardExamine g_LBExamine;

const COLORREF g_ColText = RGB( 17, 102, 221 );
const COLORREF g_ColTextHighlight = RGB( 251, 102, 0 );
const COLORREF g_ColSelected = RGB( 255, 255, 255 );
const COLORREF g_ColTextLocked = RGB( 137, 137, 137 );
const COLORREF g_ColSelectedLocked = RGB( 202, 202, 202 );
const COLORREF g_ColBlack = RGB( 0, 0, 0 );
const COLORREF g_ColWhite = RGB( 255, 255, 255 );
const COLORREF g_ColBarFG = RGB( 0, 40, 0 );
const COLORREF g_ColBarBG = RGB( 0, 212, 0 );
const COLORREF g_ColPopupBG = RGB( 212, 212, 212 );
const COLORREF g_ColPopupText = RGB( 0, 0, 40 );
const COLORREF g_ColPopupShadow = RGB( 0, 0, 0 );
const COLORREF g_ColUserFrameBG = RGB( 32, 32, 32 );
const COLORREF g_ColSelectedBoxBG = RGB( 22, 22, 60 );
const COLORREF g_ColWarning1 = RGB( 255, 0, 0 );
const COLORREF g_ColWarning2 = RGB( 80, 0, 0 );

const unsigned int OVERLAY_WIDTH = 1024;
const unsigned int OVERLAY_HEIGHT = 1024;


void AchievementOverlay::SelectNextTopLevelPage( BOOL bPressedRight )
{
	switch( m_Pages[m_nPageStackPointer] )
	{
	case OP_ACHIEVEMENTS:
		m_Pages[m_nPageStackPointer] = ( bPressedRight ? OP_FRIENDS : OP_LEADERBOARDS );
		break;
	case OP_FRIENDS:
		m_Pages[m_nPageStackPointer] = ( bPressedRight ? OP_MESSAGES : OP_ACHIEVEMENTS );
		break;
	case OP_MESSAGES:
		m_Pages[m_nPageStackPointer] = ( bPressedRight ? OP_NEWS : OP_FRIENDS );
		break;
	case OP_NEWS:
		m_Pages[m_nPageStackPointer] = ( bPressedRight ? OP_LEADERBOARDS : OP_MESSAGES );
		break;
	case OP_LEADERBOARDS:
		m_Pages[m_nPageStackPointer] = ( bPressedRight ? OP_ACHIEVEMENTS : OP_NEWS );
		break;
	default:
		//	Not on a toplevel page: cannot do anything!
		//assert(0);
		break;
	}
}

AchievementOverlay::AchievementOverlay()
{
	m_hOverlayBackground = NULL;
	m_nAchievementsSelectedItem = 0;
	m_nFriendsSelectedItem = 0;
	m_nMessagesSelectedItem = 0;
	m_nNewsSelectedItem = 0;
	m_nLeaderboardSelectedItem = 0;
}

AchievementOverlay::~AchievementOverlay()
{
	if( m_hOverlayBackground != NULL )
	{
		DeleteObject( m_hOverlayBackground );
		m_hOverlayBackground = NULL;
	}
}

void AchievementOverlay::Initialize( HINSTANCE hInst )
{
	m_nAchievementsScrollOffset = 0;
	m_nFriendsScrollOffset = 0;
	m_nMessagesScrollOffset = 0;
	m_nNewsScrollOffset = 0;
	m_nLeaderboardScrollOffset = 0;

	m_bInputLock = FALSE;
	m_nTransitionState = TS_OFF;
	m_fTransitionTimer = PAGE_TRANSITION_IN;

	m_nPageStackPointer = 0;
	m_Pages[0] = OP_ACHIEVEMENTS;
	//m_Pages.push( OP_ACHIEVEMENTS );

	m_nNumAchievementsBeingRendered = 0;
	m_nNumFriendsBeingRendered = 0;
	m_nNumLeaderboardsBeingRendered = 0;
	
	m_LatestNews.clear();

	m_hOverlayBackground = LoadLocalPNG( RA_OVERLAY_BG_FILENAME, RASize( OVERLAY_WIDTH, OVERLAY_HEIGHT ) );
	//if( m_hOverlayBackground == NULL )
	//{
	//	//	Backup
	//	m_hOverlayBackground = LoadBitmap( hInst, MAKEINTRESOURCE(IDB_RA_BACKGROUND) );
	//}
}

void AchievementOverlay::Activate()
{ 
	if( m_nTransitionState != TS_HOLD )
	{
		m_nTransitionState = TS_IN; 
		m_fTransitionTimer = PAGE_TRANSITION_IN;
	}
}

void AchievementOverlay::Deactivate()
{
	if( m_nTransitionState != TS_OFF && m_nTransitionState != TS_OUT )
	{
		m_nTransitionState = TS_OUT; 
		m_fTransitionTimer = 0.0f;

		RA_CauseUnpause();
	}
}

void AchievementOverlay::AddPage( enum OverlayPage NewPage )
{
	m_nPageStackPointer++; 
	m_Pages[m_nPageStackPointer] = NewPage;
}

//	Returns TRUE if we are ready to exit the overlay.
BOOL AchievementOverlay::GoBack()
{
	if( m_nPageStackPointer == 0 )
	{
		Deactivate();
		return TRUE;
	}
	else
	{
		m_nPageStackPointer--;
		return FALSE;
	}
}

BOOL AchievementOverlay::Update( ControllerInput* pInput, float fDelta, BOOL bFullScreen, BOOL bPaused )
{
	const int nAchCount = (const int)( g_pActiveAchievements->NumAchievements() );
	const int nNumFriends = (const int)( RAUsers::LocalUser.NumFriends() );
	const int nNumLBs = (const int)( g_LeaderboardManager.Count() );
	//const int nMsgCount = (const int)( RAUsers::LocalUser.MessageCount() );
	const int nMsgCount = 0;
	int* pnScrollOffset = const_cast<int*>( GetActiveScrollOffset() );	//	Dirty!
	int* pnSelectedItem = const_cast<int*>( GetActiveSelectedItem() );

	ControllerInput& input = *pInput;

	BOOL bCloseOverlay = FALSE;	//	False==close overlay

	//	FS fix: this thrashes horribly when both are running :S
	if( bFullScreen )
	{
		if( m_nTransitionState == TS_OUT && !bPaused )
		{
			//	Skip to 'out' if we are full-screen
			m_fTransitionTimer = PAGE_TRANSITION_OUT;
		}
	}

	if( m_nTransitionState == TS_IN )
	{
		m_fTransitionTimer += fDelta;
		
		if( m_fTransitionTimer >= 0.0f )
		{
			m_fTransitionTimer = 0.0f;
			m_nTransitionState = TS_HOLD;
		}
	}
	else if( m_nTransitionState == TS_OUT )
	{
		m_fTransitionTimer += fDelta;
		if( m_fTransitionTimer >= PAGE_TRANSITION_OUT )
		{
			m_fTransitionTimer = PAGE_TRANSITION_OUT;

			if( bPaused )
			{
				//	???
				//SelectNextTopLevelPage( TRUE );
// 				//	Still paused, just transition to another page!
// 				m_nCurrentPage = (OverlayPage)((int)(m_nCurrentPage)+1);
// 				if( m_nCurrentPage == OP__MAX )
// 					m_nCurrentPage = OP_ACHIEVEMENTS;

				m_nTransitionState = TS_IN;
				m_fTransitionTimer = PAGE_TRANSITION_IN;
			}
			else
			{
				m_nTransitionState = TS_OFF;
			}
		}
	}

	if( m_nTransitionState == TS_OFF )
		return FALSE;

	//	Inputs! Restrict to ABCULDR+Start
	if( !m_bInputLock )
	{
		switch( m_Pages[m_nPageStackPointer] )
		{
		case OP_ACHIEVEMENTS:
			{
				if( input.m_bDownPressed )
				{
					if( (*pnSelectedItem) < (nAchCount-1) )
					{
						(*pnSelectedItem)++;
						m_bInputLock = TRUE;
					}
				}
				else if( input.m_bUpPressed )
				{
					if( (*pnSelectedItem) > 0 )
					{
						(*pnSelectedItem)--;
						m_bInputLock = TRUE;
					}
				}
				else if( input.m_bConfirmPressed )
				{
					if( (*pnSelectedItem) < nAchCount )
					{
						AddPage( OP_ACHIEVEMENT_EXAMINE );
						g_AchExamine.Initialize( &g_pActiveAchievements->GetAchievement( (*pnSelectedItem) ) );
					}
				}
				
				//	Move page to match selection
				if( (*pnScrollOffset) > (*pnSelectedItem) )
					(*pnScrollOffset) = (*pnSelectedItem);
				else if( (*pnSelectedItem) > (*pnScrollOffset) + (m_nNumAchievementsBeingRendered-1) )
					(*pnScrollOffset) = (*pnSelectedItem) - (m_nNumAchievementsBeingRendered-1);

			}	
			break;
		case OP_ACHIEVEMENT_EXAMINE:
			{
				//	Overload:
				pnScrollOffset = &m_nAchievementsScrollOffset;
				pnSelectedItem = &m_nAchievementsSelectedItem;

				if( input.m_bDownPressed )
				{
					if( (*pnSelectedItem) < (nAchCount-1) )
					{
						(*pnSelectedItem)++;
						g_AchExamine.Initialize( &g_pActiveAchievements->GetAchievement( (*pnSelectedItem) ) );
						m_bInputLock = TRUE;
					}
				}
				else if( input.m_bUpPressed )
				{
					if( (*pnSelectedItem) > 0 )
					{
						(*pnSelectedItem)--;
						g_AchExamine.Initialize( &g_pActiveAchievements->GetAchievement( (*pnSelectedItem) ) );
						m_bInputLock = TRUE;
					}
				}

				//	Move page to match selection
				if( (*pnScrollOffset) > (*pnSelectedItem) )
					(*pnScrollOffset) = (*pnSelectedItem);
				else if( (*pnSelectedItem) > (*pnScrollOffset) + (m_nNumAchievementsBeingRendered-1) )
					(*pnScrollOffset) = (*pnSelectedItem) - (m_nNumAchievementsBeingRendered-1);
			}	
			break;
		case OP_FRIENDS:
			{
				if( input.m_bDownPressed )
				{
					if( (*pnSelectedItem) < (nNumFriends-1) )
					{
						(*pnSelectedItem)++;
						m_bInputLock = TRUE;
					}
				}
				else if( input.m_bUpPressed )
				{
					if( (*pnSelectedItem) > 0 )
					{
						(*pnSelectedItem)--;
						m_bInputLock = TRUE;
					}
				}

				//	Move page to match selection
				if( (*pnScrollOffset) > (*pnSelectedItem) )
					(*pnScrollOffset) = (*pnSelectedItem);
				else if( (*pnSelectedItem) > (*pnScrollOffset)+(m_nNumFriendsBeingRendered-1) )
					(*pnScrollOffset) = (*pnSelectedItem) - (m_nNumFriendsBeingRendered-1);

// 				//	Lim the selected item to a valid range
// 				while( nSelectedItem > nNumElements )
// 					nSelectedItem--;
// 
// 				if( nSelectedItem < nScrollOffset )
// 					nScrollOffset--;
// 				if( nSelectedItem >= nScrollOffset+m_nNumFriendsBeingRendered )
// 					nScrollOffset++;
			}
			break;
		case OP_MESSAGES:
			{
				//	Select message
				if( input.m_bDownPressed )
				{
					if( (*pnSelectedItem) < (nAchCount-1) )
					{
						(*pnSelectedItem)++;
						m_bInputLock = TRUE;
					}
				}
				else if( input.m_bDownPressed )
				{
					if( (*pnSelectedItem) > 0 )
					{
						(*pnSelectedItem)--;
						m_bInputLock = TRUE;
					}
				}

				//	Move page to match selection
				//if( (*pnScrollOffset) > (*pnSelectedItem) )
				//	(*pnScrollOffset) = (*pnSelectedItem);
				//else if( (*pnSelectedItem) > (*pnScrollOffset)+(NUM_MESSAGES_TO_DRAW) )
				//	(*pnScrollOffset) = (*pnSelectedItem) - (NUM_MESSAGES_TO_DRAW);
			}
			break;
		case OP_MESSAGE_VIEWER:
			{
				//RAMessage Msg = RAUsers::LocalUser.GetMessage( m_nMessagesSelectedItem );

				break;
			}
		case OP_NEWS:
			//	Scroll news
			if( input.m_bDownPressed )
			{
				if( (*pnSelectedItem) < static_cast<int>( m_LatestNews.size() ) )
				{
					(*pnSelectedItem)++;
					m_bInputLock = TRUE;
				}
			}
			else if( input.m_bUpPressed )
			{
				if( (*pnSelectedItem) > 0 )
				{
					(*pnSelectedItem)--;
					m_bInputLock = TRUE;
				}
			}
			break;
		case OP_LEADERBOARDS:
			//	Scroll news
			if( input.m_bDownPressed )
			{
				if( (*pnSelectedItem) < (nNumLBs-1) )
				{
					(*pnSelectedItem)++;
					m_bInputLock = TRUE;
				}
			}
			else if( input.m_bUpPressed )
			{
				if( (*pnSelectedItem) > 0 )
				{
					(*pnSelectedItem)--;
					m_bInputLock = TRUE;
				}
			}
			if( input.m_bConfirmPressed )
			{
				if( (*pnSelectedItem) < nNumLBs )
				{
					AddPage( OP_LEADERBOARD_EXAMINE );
					g_LBExamine.Initialize( g_LeaderboardManager.GetLB( (*pnSelectedItem) ).ID() );
				}
			}
			//	Move page to match selection
			if( (*pnScrollOffset) > (*pnSelectedItem) )
				(*pnScrollOffset) = (*pnSelectedItem);
			else if( (*pnSelectedItem) > (*pnScrollOffset) + (m_nNumLeaderboardsBeingRendered-1) )
				(*pnScrollOffset) = (*pnSelectedItem) - (m_nNumLeaderboardsBeingRendered-1);
			break;
		case OP_LEADERBOARD_EXAMINE:
			//	Overload from previous
			//	Overload:
			pnScrollOffset = &m_nLeaderboardScrollOffset;
			pnSelectedItem = &m_nLeaderboardSelectedItem;

			if( input.m_bDownPressed )
			{
				if( (*pnSelectedItem) < (nNumLBs-1) )
				{
					(*pnSelectedItem)++;
					g_LBExamine.Initialize( g_LeaderboardManager.GetLB( (*pnSelectedItem) ).ID() );
					m_bInputLock = TRUE;
				}
			}
			else if( input.m_bUpPressed )
			{
				if( (*pnSelectedItem) > 0 )
				{
					(*pnSelectedItem)--;
					g_LBExamine.Initialize( g_LeaderboardManager.GetLB( (*pnSelectedItem) ).ID() );
					m_bInputLock = TRUE;
				}
			}

			//	Move page to match selection
			//if( (*pnScrollOffset) > (*pnSelectedItem) )
			//	(*pnScrollOffset) = (*pnSelectedItem);
			//else if( (*pnSelectedItem) > (*pnScrollOffset) + (m_nNumAchievementsBeingRendered-1) )
			//	(*pnScrollOffset) = (*pnSelectedItem) - (m_nNumAchievementsBeingRendered-1);

			break;
		default:
			assert(0);	//	Unknown page!
			break;
		}
		
		
		if( input.m_bCancelPressed )
		{
			//	If TRUE: Close overlay
			bCloseOverlay = GoBack();
			m_bInputLock = TRUE;
		}

		if( input.m_bLeftPressed || input.m_bRightPressed )
		{
			if( m_nTransitionState == TS_HOLD )
			{
				SelectNextTopLevelPage( input.m_bRightPressed );
				m_bInputLock = TRUE;
			}
		}
	}
	else
	{
		if( !input.m_bUpPressed && !input.m_bDownPressed && !input.m_bLeftPressed && 
			!input.m_bRightPressed && !input.m_bConfirmPressed && !input.m_bCancelPressed )
		{
			m_bInputLock = FALSE;
		}
	}

	if( input.m_bQuitPressed )
	{
		Deactivate();
		bCloseOverlay = TRUE;
	}

	return bCloseOverlay;
}

void AchievementOverlay::DrawAchievementsPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const
{
	const int nGameTitleX = 12;
	const int nGameTitleY = 80;
	const int nGameSubTitleY = nGameTitleY+36;

	const int nNumAchievements = g_pActiveAchievements->NumAchievements();

	unsigned int nMaxPts = 0;
	unsigned int nUserPts = 0;
	unsigned int nUserCompleted = 0;
	int i = 0;
	Achievement* pAch = NULL;

	const int nAchTopEdge = 160;//80;
	const int nAchSpacing = 64+8;//52;
	const int nAchImageOffset = 28;
	int nAchIdx = 0;
	char buffer[256];

	const int* pnScrollOffset = GetActiveScrollOffset();
	const int* pnSelectedItem = GetActiveSelectedItem();

	const int nWidth = rcTarget.right - rcTarget.left;
	const int nHeight = rcTarget.bottom - rcTarget.top;

	if( g_pActiveAchievements->GameTitle().length() > 0 )
	{
		SelectObject( hDC, g_hFontTitle );
		SetTextColor( hDC, g_ColText );
		sprintf_s( buffer, 256, " %s ", g_pActiveAchievements->GameTitle().c_str() );
		TextOut( hDC, nGameTitleX+nDX, nGameTitleY+nDY, buffer, strlen( buffer ) );
	}

	SelectObject( hDC, g_hFontDesc );
	if( g_nActiveAchievementSet == AchievementSetCore )
	{
		for( i = 0; i < nNumAchievements; ++i )
		{
			Achievement* pAch = &g_pActiveAchievements->GetAchievement( i );
			nMaxPts += pAch->Points();
			if( !pAch->Active() )
			{
				nUserPts += pAch->Points();
				nUserCompleted++;
			}
		}

		if( nNumAchievements > 0 )
		{
			SetTextColor( hDC, g_ColTextLocked );
			sprintf_s( buffer, 256, " %d of %d won (%d/%d) ",
				nUserCompleted, nNumAchievements,
				nUserPts, nMaxPts );
			TextOut( hDC, nDX+nGameTitleX, nGameSubTitleY, buffer, strlen( buffer ) );
		}
	}

	int nAchievementsToDraw = ( (rcTarget.bottom - rcTarget.top) - 160 ) / nAchSpacing;

	if( nAchievementsToDraw > 0 && nNumAchievements > 0 )
	{
		for( int i = 0; i < nAchievementsToDraw; ++i )
		{
			nAchIdx = (*pnScrollOffset)+i;
			if( nAchIdx < (int)nNumAchievements )
			{
				BOOL bSelected = ( (*pnSelectedItem) - (*pnScrollOffset) == i );
				if( bSelected )
				{
					//	Draw bounding box around text
					const int nSelBoxXOffs = 28+64;
					const int nSelBoxWidth = nWidth - nAchSpacing - 24;
					const int nSelBoxHeight = nAchSpacing - 8;

					RECT rcSelected;
					SetRect( &rcSelected, nDX + nSelBoxXOffs,					(nAchTopEdge + (i*nAchSpacing)), 
						nDX + nSelBoxXOffs + nSelBoxWidth,	(nAchTopEdge + (i*nAchSpacing))+nSelBoxHeight );
					FillRect( hDC, &rcSelected, g_hBrushSelectedBG );
				}

				DrawAchievement( hDC, 
					&g_pActiveAchievements->GetAchievement( nAchIdx ),	//	pAch
					nDX,												//	X
					(nAchTopEdge + (i*nAchSpacing)),					//	Y
					bSelected,											//	Selected
					TRUE );
			}
		}

		if( nNumAchievements > (nAchievementsToDraw-1) )
		{
			DrawBar( hDC, 
				nDX+8, 
				nAchTopEdge, 
				12, 
				nAchSpacing*nAchievementsToDraw, 
				nNumAchievements - (nAchievementsToDraw-1), 
				(*pnScrollOffset) );
		}
	}
	else
	{

		if( !RA_GameIsActive() )
		{
			sprintf_s( buffer, 256, " No game loaded " );
			TextOut( hDC, nDX+nGameTitleX, nGameSubTitleY, buffer, strlen( buffer ) );
		}
		else if( nNumAchievements == 0 )
		{
			sprintf_s( buffer, 256, " No achievements present... " );
			TextOut( hDC, nDX+nGameTitleX, nGameSubTitleY, buffer, strlen( buffer ) );
		}
	}

	m_nNumAchievementsBeingRendered = nAchievementsToDraw;
}

void AchievementOverlay::DrawMessagesPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const
{

	// 		for( size_t i = 0; i < 256; ++i )
	// 			buffer[i] = (char)(i);
	// 
	// 		SelectObject( hDC, hFontDesc );
	// 		TextOut( hDC, nDX+8, 40, buffer, 32 );
	// 		TextOut( hDC, nDX+8, 60, buffer+32, 32 );
	// 		TextOut( hDC, nDX+8, 80, buffer+64, 32 );
	// 		TextOut( hDC, nDX+8, 100, buffer+96, 32 );
	// 		TextOut( hDC, nDX+8, 120, buffer+128, 32 );
	// 		TextOut( hDC, nDX+8, 140, buffer+160, 32 );
	// 		TextOut( hDC, nDX+8, 160, buffer+192, 32 );
	// 		TextOut( hDC, nDX+8, 180, buffer+224, 32 );

}

void AchievementOverlay::DrawFriendsPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const
{
	const int* pnScrollOffset = GetActiveScrollOffset();
	const int* pnSelectedItem = GetActiveSelectedItem();

	const unsigned int nFriendSpacing = 64+8;//80;
	const unsigned int nFriendsToDraw = ( (rcTarget.bottom - rcTarget.top) - 140 ) / nFriendSpacing;

	const unsigned int nFriendLeftOffsetImage = 32;//16;
	const unsigned int nFriendLeftOffsetText = 84;//64;

	const unsigned int nFriendTopEdge = 120;//44;
	const unsigned int nFriendSubtitleYOffs = 24;
	const unsigned int nFriendSpacingText = 36;//18;

	char buffer[256];

	unsigned int nOffset = m_nFriendsScrollOffset;

	const unsigned int nNumFriends = RAUsers::LocalUser.NumFriends();

	for( unsigned int i = 0; i < nFriendsToDraw; ++i )
	{
		int nXOffs = nDX+( rcTarget.left + nFriendLeftOffsetImage );
		int nYOffs = nFriendTopEdge+nFriendSpacing*i;

		if( i > nNumFriends )
			break;

		if( (i+nOffset) < nNumFriends )
		{
			RAUser* pFriend = RAUsers::LocalUser.GetFriendByIter( (i+nOffset) );
			if( pFriend == NULL )
				continue;

			if( pFriend->GetUserImage() == NULL && !pFriend->IsFetchingUserImage() )
				pFriend->LoadOrFetchUserImage();

			if( pFriend->GetUserImage() != NULL )
				DrawImage( hDC, pFriend->GetUserImage(), nXOffs, nYOffs, 64, 64 );

			if( (m_nFriendsSelectedItem - m_nFriendsScrollOffset) == i )
			{
				SetTextColor( hDC, g_ColSelected );
			}
			else
			{
				SetTextColor( hDC, g_ColText );
			}

			HANDLE hOldObj = SelectObject( hDC, g_hFontDesc );

			sprintf_s( buffer, 256, " %s (%d) ", pFriend->Username(), pFriend->GetScore() );
			TextOut( hDC, nXOffs+nFriendLeftOffsetText, nYOffs, buffer, strlen( buffer ) );

			SelectObject( hDC, g_hFontTiny );
			sprintf_s( buffer, 256, " %s ", pFriend->Activity() );
			RECT rcDest;
			SetRect( &rcDest, 
				nXOffs+nFriendLeftOffsetText, 
				nYOffs+nFriendSubtitleYOffs, 
				nDX + rcTarget.right - 40,
				nYOffs+nFriendSubtitleYOffs + 46 );
			DrawText( hDC, pFriend->Activity().c_str(), -1, &rcDest, DT_LEFT|DT_WORDBREAK );

			//	Restore
			SelectObject( hDC, hOldObj );

			//sprintf_s( buffer, 256, " %d Points ", Friend.Score() );
			//TextOut( hDC, nXOffs+nFriendLeftOffsetText, nYOffs+nFriendSpacingText, buffer, strlen( buffer ) );
		}
	}

	if( nNumFriends > (nFriendsToDraw) )
	{
		DrawBar( hDC, 
			nDX+8, 
			nFriendTopEdge, 
			12, 
			nFriendSpacing*nFriendsToDraw, 
			nNumFriends - (nFriendsToDraw-1), 
			(*pnScrollOffset) );
	}

	m_nNumFriendsBeingRendered = nFriendsToDraw;
}

void AchievementOverlay::DrawAchievementExaminePage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const
{
	char buffer[256];

	const int* pnScrollOffset = GetActiveScrollOffset();
	const int* pnSelectedItem = GetActiveSelectedItem();

	const unsigned int nNumAchievements = g_pActiveAchievements->NumAchievements();

	const int nAchievementStartX = 0;
	const int nAchievementStartY = 80;

	const int nLoadingMessageX = 120;
	const int nLoadingMessageY = 300;

	const int nCoreDetailsY = 150;
	const int nCoreDetailsSpacing = 32;

	const int nWonByPlayerYOffs = 300;
	const int nWonByPlayerYSpacing = 28;

	const int nRecentWinnersSubtitleX = 20;
	const int nRecentWinnersSubtitleY = 250;

	const int nWonByPlayerNameX = 20;
	const int nWonByPlayerDateX = 180;

	char bufTime[256];

	Achievement* pAch = &g_pActiveAchievements->GetAchievement( m_nAchievementsSelectedItem );

	const time_t tCreated = pAch->CreatedDate();
	const time_t tModified = pAch->ModifiedDate();

	DrawAchievement( hDC, 
		pAch, 
		nDX+nAchievementStartX, 
		nAchievementStartY, 
		TRUE, 
		FALSE );

	if( m_nAchievementsSelectedItem >= (int)nNumAchievements )
		return;

	ctime_s( bufTime, 256, &tCreated );
	bufTime[strlen(bufTime)-1] = '\0';	//	Remove pesky newline
	sprintf_s( buffer, 256, " Created: %s ", bufTime );
	TextOut( hDC, nDX+20, nCoreDetailsY, buffer, strlen( buffer ) );

	ctime_s( bufTime, 256, &tModified );
	bufTime[strlen(bufTime)-1] = '\0';	//	Remove pesky newline
	sprintf_s( buffer, 256, " Modified: %s ", bufTime );
	TextOut( hDC, nDX+20, nCoreDetailsY+nCoreDetailsSpacing, buffer, strlen( buffer ) );

	if( g_AchExamine.HasData() )
	{
		sprintf_s( buffer, 256, " Won by %d of %d ", g_AchExamine.TotalWinners(), g_AchExamine.PossibleWinners() );
		TextOut( hDC, nDX+20, nCoreDetailsY+(nCoreDetailsSpacing*2), buffer, strlen( buffer ) );

		if( g_AchExamine.NumRecentWinners() > 0 )
		{
			sprintf_s( buffer, 256, " Recent winners: " );
			TextOut( hDC, nDX+nRecentWinnersSubtitleX, nRecentWinnersSubtitleY, buffer, strlen( buffer ) );
		}

		for( unsigned int i = 0; i < g_AchExamine.NumRecentWinners(); ++i )
		{
			const AchievementExamine::RecentWinnerData& data = g_AchExamine.GetRecentWinner( i );

			char buffer[256];
			sprintf_s( buffer, 256, " %s ", data.User().c_str() );

			char buffer2[256];
			sprintf_s( buffer2, 256, " %s ", data.WonAt().c_str() );

			//	Draw/Fetch user image? //TBD

			TextOut( hDC, 
				nDX+nWonByPlayerNameX, 
				nWonByPlayerYOffs+(i*nWonByPlayerYSpacing), 
				buffer, strlen( buffer ) );

			TextOut( hDC, 
				nDX+nWonByPlayerDateX, 
				nWonByPlayerYOffs+(i*nWonByPlayerYSpacing), 
				buffer2, strlen( buffer2 ) );
		}
	}
	else
	{
		int nDotCount = 0;
		static int nDots = 0;
		nDots++;
		if( nDots > 100 )
			nDots = 0;

		nDotCount = nDots / 25;

		sprintf_s( buffer, 256, " Loading.%c%c%c ", 
			nDotCount > 1 ? '.' : ' ',
			nDotCount > 2 ? '.' : ' ',
			nDotCount > 3 ? '.' : ' ' );

		TextOut( hDC, nDX+nLoadingMessageX, nLoadingMessageY, buffer, strlen( buffer ) );
	}
}

void AchievementOverlay::DrawNewsPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const
{
	unsigned int nYOffset = 90;

	const unsigned int nHeaderGap = 4;
	const unsigned int nArticleGap = 10;

	const unsigned int nBorder = 32;

	const unsigned int nLeftAlign = nDX+nBorder;
	const unsigned int nRightAlign = nDX+(rcTarget.right - nBorder);

	const unsigned int nArticleIndent = 20;

	const int nWidth = rcTarget.right - rcTarget.left;
	const int nHeight = rcTarget.bottom - rcTarget.top;

	HGDIOBJ hOldObject = SelectObject( hDC, g_hFontDesc2 );

	for( int i = m_nNewsSelectedItem; i < static_cast<int>( m_LatestNews.size() ); ++i )
	{
		const char* sTitle = m_LatestNews[i].m_sTitle.c_str();
		const char* sPayload = m_LatestNews[i].m_sPayload.c_str();

		RECT rcNews;

		SelectObject( hDC, g_hFontDesc2 );

		//	Setup initial variables for the rect
		SetRect( &rcNews, nLeftAlign, nYOffset, nRightAlign, nYOffset );
		//	Calculate height of rect, fetch bottom for next rect:
		DrawText( hDC, sTitle, strlen(sTitle), &rcNews, DT_CALCRECT|DT_WORDBREAK );
		nYOffset = rcNews.bottom;

		if( rcNews.bottom > nHeight )
			rcNews.bottom = nHeight;

		//	Draw title:
		DrawText( hDC, sTitle, strlen(sTitle), &rcNews, DT_TOP|DT_LEFT|DT_WORDBREAK );

		//	Offset header
		nYOffset += nHeaderGap;

		SelectObject( hDC, g_hFontTiny );

		//	Setup initial variables for the rect
		SetRect( &rcNews, nLeftAlign+nArticleIndent, nYOffset, nRightAlign-nArticleIndent, nYOffset );
		//	Calculate height of rect, fetch and inset bottom:
		DrawText( hDC, sPayload, strlen(sPayload), &rcNews, DT_CALCRECT|DT_WORDBREAK );
		nYOffset = rcNews.bottom;

		if( rcNews.bottom > nHeight )
			rcNews.bottom = nHeight;

		//	Draw payload:
		DrawText( hDC, sPayload, strlen(sPayload), &rcNews, DT_TOP|DT_LEFT|DT_WORDBREAK );

		nYOffset += nArticleGap;
	}

	//	Restore object:
	SelectObject( hDC, hOldObject );
}

void AchievementOverlay::DrawLeaderboardPage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const
{
	const unsigned int nYOffsetTop = 90;
	unsigned int nYOffset = nYOffsetTop;

	const unsigned int nHeaderGap = 4;
	const unsigned int nArticleGap = 10;

	const unsigned int nBorder = 32;

	const unsigned int nLeftAlign = nDX+nBorder;
	const unsigned int nRightAlign = nDX+(rcTarget.right - nBorder);

	const unsigned int nArticleIndent = 20;

	const int nWidth = rcTarget.right - rcTarget.left;
	const int nHeight = rcTarget.bottom - rcTarget.top;

	const int* pnScrollOffset = GetActiveScrollOffset();
	const int* pnSelectedItem = GetActiveSelectedItem();

	const int nLBSpacing = 52;

	const int nItemSpacing = 48;

	HGDIOBJ hOldObject = SelectObject( hDC, g_hFontDesc2 );

	m_nNumLeaderboardsBeingRendered = 0;

	unsigned int nNumLBsToDraw = ( (rcTarget.bottom - rcTarget.top) - 160 ) / nItemSpacing;
	unsigned int nNumLBs = g_LeaderboardManager.Count();

	if( nNumLBsToDraw > nNumLBs )
		nNumLBsToDraw = nNumLBs;

	if( nNumLBs > 0 && nNumLBsToDraw > 0 )
	{
		for( unsigned int i = m_nLeaderboardScrollOffset; i < m_nLeaderboardScrollOffset+nNumLBsToDraw; ++i )
		{
			if( i >= g_LeaderboardManager.Count() )
				continue;

			RA_Leaderboard& nextLB = g_LeaderboardManager.GetLB( i );

			const char* sTitle = nextLB.Title().c_str();
			const char* sPayload = nextLB.Description().c_str();
			
			BOOL bSelected = ( (*pnSelectedItem) == i );
			if( bSelected )
			{
				//	Draw bounding box around text
				const int nSelBoxXOffs = nLeftAlign - 4;
				const int nSelBoxWidth = nWidth - 8;
				const int nSelBoxHeight = nItemSpacing;

				RECT rcSelected;
				SetRect( &rcSelected, 
							nDX + nSelBoxXOffs,		
							nYOffset, 
							nDX + nSelBoxXOffs + nSelBoxWidth,
							nYOffset+nSelBoxHeight );

				FillRect( hDC, &rcSelected, g_hBrushSelectedBG );
			}

			SelectObject( hDC, g_hFontDesc2 );

			//	Setup initial variables for the rect
			RECT rcNews;
			SetRect( &rcNews, nLeftAlign, nYOffset, nRightAlign, nYOffset );
			//	Calculate height of rect, fetch bottom for next rect:
			DrawText( hDC, sTitle, strlen(sTitle), &rcNews, DT_CALCRECT|DT_WORDBREAK );
			nYOffset = rcNews.bottom;

			if( rcNews.bottom > nHeight )
				rcNews.bottom = nHeight;

			
			if( bSelected )
				SetTextColor( hDC, g_ColSelected );
			else
				SetTextColor( hDC, g_ColText );

			//	Draw title:
			DrawText( hDC, sTitle, strlen(sTitle), &rcNews, DT_TOP|DT_LEFT|DT_WORDBREAK );

			//	Offset header
			nYOffset += nHeaderGap;

			SelectObject( hDC, g_hFontTiny );

			//	Setup initial variables for the rect
			SetRect( &rcNews, nLeftAlign+nArticleIndent, nYOffset, nRightAlign-nArticleIndent, nYOffset );
			//	Calculate height of rect, fetch and inset bottom:
			DrawText( hDC, sPayload, strlen(sPayload), &rcNews, DT_CALCRECT|DT_WORDBREAK );
			nYOffset = rcNews.bottom;

			if( rcNews.bottom > nHeight )
				rcNews.bottom = nHeight;

			SetTextColor( hDC, g_ColSelectedLocked );

			//	Draw payload:
			DrawText( hDC, sPayload, strlen(sPayload), &rcNews, DT_TOP|DT_LEFT|DT_WORDBREAK );

			nYOffset += nArticleGap;

			m_nNumLeaderboardsBeingRendered++;
		}

		if( nNumLBs > (nNumLBsToDraw-1) )
		{
			DrawBar( hDC, 
				nDX+8, 
				nYOffsetTop, 
				12, 
				nLBSpacing*m_nNumLeaderboardsBeingRendered, 
				nNumLBs - (nNumLBsToDraw-1),
				(*pnScrollOffset) );
		}
	}

	//	Restore object:
	SelectObject( hDC, hOldObject );
}

void AchievementOverlay::DrawLeaderboardExaminePage( HDC hDC, int nDX, int nDY, const RECT& rcTarget ) const
{
	char buffer[256];

	const int* pnScrollOffset = GetActiveScrollOffset();
	const int* pnSelectedItem = GetActiveSelectedItem();

	const int nLBStartX = 0;
	const int nLBStartY = 80;

	const int nLoadingMessageX = 120;
	const int nLoadingMessageY = 300;

	const int nCoreDetailsY = 100;
	const int nCoreDetailsSpacing = 28;

	const int nLeaderboardYOffs = 160;
	const int nLeaderboardYSpacing = 26;

	const int nRecentWinnersSubtitleX = 20;
	const int nRecentWinnersSubtitleY = 272;

	const int nWonByPlayerRankX = 20;
	const int nWonByPlayerUserX = 100;
	const int nWonByPlayerScoreX = 320;

	//char bufTime[256];

	const RA_Leaderboard* pLB = g_LeaderboardManager.FindLB( g_LBExamine.m_nLBID );
	if( pLB == NULL )
	{
		TextOut( hDC, 120, 120, "No leaderboard found", strlen("No leaderboard found") );
	}
	else
	{
		sprintf_s( buffer, 256, " %s ", pLB->Title() );
		TextOut( hDC, nDX+20, nCoreDetailsY, buffer, strlen( buffer ) );

		SetTextColor( hDC, g_ColSelectedLocked );
		sprintf_s( buffer, 256, " %s ", pLB->Description() );
		TextOut( hDC, nDX+20, nCoreDetailsY+nCoreDetailsSpacing, buffer, strlen( buffer ) );

		SetTextColor( hDC, g_ColText );

		if( g_LBExamine.m_bHasData )
		{
			//sprintf_s( buffer, 256, " Won by %d of %d ", g_AchExamine.m_nTotalWinners, g_AchExamine.m_nPossibleWinners );
			//TextOut( hDC, nDX+20, nCoreDetailsY+(nCoreDetailsSpacing*2), buffer, strlen( buffer ) );

			//if( pLB->GetRankInfoCount() > 0 )
			//{
			//	sprintf_s( buffer, 256, " Recent winners: " );
			//	TextOut( hDC, nDX+nRecentWinnersSubtitleX, nRecentWinnersSubtitleY, buffer, strlen( buffer ) );
			//}
			
			for( size_t i = 0; i < pLB->GetRankInfoCount(); ++i )
			{
				const LB_Entry& rEntry = pLB->GetRankInfo( i );
				//const char* sWinnerDate = rEntry.m_TimeAchieved;	//	deal another time
				
				unsigned int nScore = rEntry.m_nScore;
				char bufferScoreFormatted[256];
				RA_Leaderboard::FormatScore( pLB->GetFormatType(), nScore, bufferScoreFormatted, 256 );

				char sRankText[256];
				sprintf_s( sRankText, 256, " %d ", rEntry.m_nRank );

				char sNameText[256];
				sprintf_s( sNameText, 256, " %s ", rEntry.m_sUsername );

				char sScoreText[256];
				sprintf_s( sScoreText, 256, " %s ", bufferScoreFormatted );

				//	Draw/Fetch user image? //TBD

				TextOut( hDC, 
					nDX+nWonByPlayerRankX, 
					nLeaderboardYOffs+(i*nLeaderboardYSpacing), 
					sRankText, strlen( sRankText ) );

				TextOut( hDC, 
					nDX+nWonByPlayerUserX, 
					nLeaderboardYOffs+(i*nLeaderboardYSpacing), 
					sNameText, strlen( sNameText ) );

				TextOut( hDC, 
					nDX+nWonByPlayerScoreX, 
					nLeaderboardYOffs+(i*nLeaderboardYSpacing), 
					sScoreText, strlen( sScoreText ) );
			}
		}
		else
		{
			int nDotCount = 0;
			static int nDots = 0;
			nDots++;
			if( nDots > 100 )
				nDots = 0;

			nDotCount = nDots / 25;

			sprintf_s( buffer, 256, " Loading.%c%c%c ", 
				nDotCount > 1 ? '.' : ' ',
				nDotCount > 2 ? '.' : ' ',
				nDotCount > 3 ? '.' : ' ' );

			TextOut( hDC, nDX+nLoadingMessageX, nLoadingMessageY, buffer, strlen( buffer ) );
		}	
	}
}

void AchievementOverlay::Render( HDC hDC, RECT* rcDest ) const
{
	//	Rendering:
	if( !RAUsers::LocalUser.IsLoggedIn() )
		return;	//	Not available!

	const COLORREF nPrevTextColor = GetTextColor( hDC );
	const COLORREF nPrevBkColor = GetBkColor( hDC );
	const int nOuterBorder = 4;
	RECT rcTarget = (*rcDest);
	int nBorder = 8;

	const int nFontSize1 = 32;
	const int nFontSize2 = 26;
	const int nFontSize3 = 22;
	const int nFontSize4 = 16;

	const int nPixelWidth = (*rcDest).right - (*rcDest).left;
	const BOOL bHiRes = ( nPixelWidth >= 320 );

	unsigned int nMinUserFrameWidth = 300;
	unsigned int nMinUserFrameHeight = 64+4+4;
	char buffer[1024];

	HBRUSH hBrush = NULL;

	if( m_nTransitionState == TS_OFF )
		return;

	const int nWidth = rcTarget.right - rcTarget.left;
	const int nHeight = rcTarget.bottom - rcTarget.top;
	
	
	g_hFontTitle = CreateFont( nFontSize1, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_CHARACTER_PRECIS, /*NON*/ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT(FONT_TO_USE) );

	g_hFontDesc = CreateFont( nFontSize2, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, 
		CLIP_CHARACTER_PRECIS, /*NON*/ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT(FONT_TO_USE) );

	g_hFontDesc2 = CreateFont( nFontSize3, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_CHARACTER_PRECIS, /*NON*/ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT(FONT_TO_USE) );

	g_hFontTiny = CreateFont( nFontSize4, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
		CLIP_CHARACTER_PRECIS, /*NON*/ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT(FONT_TO_USE) );

	float fPctOffScreen = ( m_nTransitionState == TS_IN ) ?
							( m_fTransitionTimer / PAGE_TRANSITION_IN ) : 
							( m_fTransitionTimer / PAGE_TRANSITION_OUT );

	int nDX = (int)( 0 - ( fPctOffScreen * (rcTarget.right-rcTarget.left) ) );
	int nDY = rcTarget.top;

	int nRightPx = (int)( rcTarget.right - ( fPctOffScreen * rcTarget.right ) );
	int nRightPxAbs = (int)( (rcTarget.right-rcTarget.left) - ( fPctOffScreen * (rcTarget.right-rcTarget.left) ) );

	RECT rc;
	SetRect( &rc, 
			 nDX,
			 nDY, 
			 nDX+rcTarget.right, 
			 rcTarget.bottom );
	
	//	Draw background:
	int nOldBkMode = SetBkMode( hDC, TRANSPARENT );
	
	RECT rcBGSize;
	SetRect( &rcBGSize, 0, 0, OVERLAY_WIDTH, OVERLAY_HEIGHT );
	OffsetRect( &rcBGSize, -((LONG)OVERLAY_WIDTH-rc.right), 0 );
	DrawImageTiled( hDC, m_hOverlayBackground, rcBGSize, rc );

	g_hBrushBG			= CreateSolidBrush( g_ColUserFrameBG );
	g_hBrushSelectedBG	= CreateSolidBrush( g_ColSelectedBoxBG );

	SetTextColor( hDC, g_ColText );
	SelectObject( hDC, g_hFontDesc );

	//	Draw user info
	if( rcTarget.right > 360 )
	{
		DrawUserFrame( 
			hDC, &RAUsers::LocalUser,
			( nDX+(rcTarget.right - nMinUserFrameWidth) )-4,
			4+nBorder, 
			nMinUserFrameWidth, 
			nMinUserFrameHeight );
	}


	//	Draw the bulk of the page:
	const OverlayPage currentPage = m_Pages[m_nPageStackPointer];
	switch( currentPage )
	{
	case OP_ACHIEVEMENTS:
		{
			DrawAchievementsPage( hDC, nDX, nDY, rcTarget );
		}
		break;
	case OP_MESSAGES:
		{
			DrawMessagesPage( hDC, nDX, nDY, rcTarget );
		}
		break;
	case OP_FRIENDS:
		{
			DrawFriendsPage( hDC, nDX, nDY, rcTarget );
		}
		break;
	case OP_ACHIEVEMENT_EXAMINE:
		{
			DrawAchievementExaminePage( hDC, nDX, nDY, rcTarget );
		}
		break;
	case OP_NEWS:
		{
			DrawNewsPage( hDC, nDX, nDY, rcTarget );
		}
		break;
	case OP_LEADERBOARDS:
		{
			DrawLeaderboardPage( hDC, nDX, nDY, rcTarget );
		}
		break;
	case OP_LEADERBOARD_EXAMINE:
		{
			DrawLeaderboardExaminePage( hDC, nDX, nDY, rcTarget );
		}
		break;
	default:
		//	Not implemented!
		assert(!"Attempting to render an undefined overlay page!" );
		break;
	}

	const int* pnScrollOffset = GetActiveScrollOffset();
	const int* pnSelectedItem = GetActiveSelectedItem();

	//	Title:
	SelectObject( hDC, g_hFontTitle );
	SetTextColor( hDC, g_ColText );
	sprintf_s( buffer, 1024, g_sPageTitles[ (int)currentPage ], (*pnScrollOffset)+1 );
	TextOut( hDC, 
		nDX+nBorder, 
		4+nBorder, 
		buffer, strlen( buffer ) );


	//int nNextPage = (int)(m_nCurrentPage+1);
	//if( nNextPage == (int)OP__MAX )
	//	nNextPage = (int)OP_ACHIEVEMENTS;
// 	sprintf_s( buffer, 1024, " A:%s", g_sPageTitles[nNextPage] );
// 	TextOut( hDC, nDX+8, nHeight-24, buffer, strlen( buffer ) );
	
	//	Render controls:
	SelectObject( hDC, g_hFontDesc2 );
	{
		const int nControlsX1 = 80+80+4;
		const int nControlsX2 = 80;
		const int nControlsY1 = rcTarget.bottom-30-30-4;
		const int nControlsY2 = rcTarget.bottom-30-4;
		
		//	Fill again:
		SetRect( &rc, nRightPx-nControlsX1-4, nControlsY1-4, nRightPx, nHeight );
		FillRect( hDC, &rc, g_hBrushBG );

		//	Draw control text:
		sprintf_s( buffer, 1024, " ->:%s ", "Next" );
		TextOut( hDC, nRightPx-nControlsX1, nControlsY1, buffer, strlen( buffer ) );

		sprintf_s( buffer, 1024, " <-:%s ", "Prev" );
		TextOut( hDC, nRightPx-nControlsX1, nControlsY2, buffer, strlen( buffer ) );

		char cBackChar = 'B';
		char cSelectChar = 'A';
		
		if( g_EmulatorID == RA_Gens )
		{
			//	Genesis wouldn't use 'A' for select
			cSelectChar = 'C';
		}

		sprintf_s( buffer, 1024, " %c:%s ", cBackChar, "Back" );
		TextOut( hDC, nRightPx-nControlsX2, nControlsY1, buffer, strlen( buffer ) );

		sprintf_s( buffer, 1024, " %c:%s ", cSelectChar, "Select" );
		TextOut( hDC, nRightPx-nControlsX2, nControlsY2, buffer, strlen( buffer ) );
	}

	DeleteObject(g_hBrushBG);
	DeleteObject(g_hBrushSelectedBG);
	DeleteObject(g_hFontTitle);
	DeleteObject(g_hFontDesc);
	DeleteObject(g_hFontDesc2);
	DeleteObject(g_hFontTiny);
	
	SetBkColor( hDC, nPrevBkColor );
	SetTextColor( hDC, nPrevTextColor );
	SetBkMode( hDC, nOldBkMode );
}

void AchievementOverlay::DrawBar( HDC hDC, int nX, int nY, int nW, int nH, int nMax, int nSel ) const
{
	HBRUSH hBarBack = (HBRUSH)GetStockObject( DKGRAY_BRUSH );
	HBRUSH hBarFront = (HBRUSH)GetStockObject( LTGRAY_BRUSH );
	RECT rc;
	float fNumMax = (float)( nMax );
	const float fInnerBarMaxSizePx = (float)nH - 4.0f;

	const float fInnerBarSizePx = ( fInnerBarMaxSizePx / fNumMax );
	const float fInnerBarOffsetY = fInnerBarSizePx * nSel;

	const int nInnerBarAbsY = (int)( nY+2.0f+fInnerBarOffsetY );

	//	Draw bar:
	SetBkColor( hDC, g_ColBarBG );
	SetTextColor( hDC, g_ColBarFG );

	SetRect( &rc, nX, nY, nX+nW, nY+nH );
	if( FillRect( hDC, &rc, hBarBack ) )
	{
		SetTextColor( hDC, g_ColText );

		if( fNumMax <= 0.0f )
			fNumMax = 1.0f;

		SetRect( &rc, nX+2, nInnerBarAbsY, nX+(nW-2), nInnerBarAbsY+(int)(fInnerBarSizePx) );
		FillRect( hDC, &rc, hBarFront );
	}

	DeleteObject( hBarFront );
	DeleteObject( hBarBack );
}

void AchievementOverlay::DrawAchievement( HDC hDC, const Achievement* pAch, int nX, int nY, BOOL bSelected, BOOL bCanLock ) const
{
	const int nAchImageOffset = 28;
	const int nAchLeftOffset1 = 28+64+6;
	const int nAchLeftOffset2 = 28+64+6+4;
	const int nAchSpacingDesc = 24;
	BOOL bLocked = FALSE;
	char buffer[1024];

	if( bCanLock )
	{
		if( g_nActiveAchievementSet == AchievementSetCore )
			bLocked = pAch->Active();
	}

	if( bSelected )
	{
		if( bLocked )
			SetTextColor( hDC, g_ColSelectedLocked );
		else
			SetTextColor( hDC, g_ColSelected );
	}
	else
	{
		if( bLocked )
			SetTextColor( hDC, g_ColTextLocked );
		else
			SetTextColor( hDC, g_ColText );
	}

	if( !bLocked )
	{
		if( pAch->BadgeImage() != NULL )
			DrawImage( hDC, pAch->BadgeImage(), nX+nAchImageOffset, nY, 64, 64 );
	}
	else
	{
		if( pAch->BadgeImageLocked() != NULL )
			DrawImage( hDC, pAch->BadgeImageLocked(), nX+nAchImageOffset, nY, 64, 64 );
	}

	sprintf_s( buffer, 1024, " %s ", pAch->Description().c_str() );
	SelectObject( hDC, g_hFontDesc2 );
	TextOut( hDC, nX+nAchLeftOffset2, nY+nAchSpacingDesc, buffer, strlen( buffer ) );

	sprintf_s( buffer, 1024, " %s (%d Points) ", pAch->Title().c_str(), pAch->Points() );
	SelectObject( hDC, g_hFontDesc );
	TextOut( hDC, nX+nAchLeftOffset1, nY, buffer, strlen( buffer ) );
}

void AchievementOverlay::DrawUserFrame( HDC hDC, RAUser* pUser, int nX, int nY, int nW, int nH ) const
{
	char buffer[256];
	HBRUSH hBrush2 = CreateSolidBrush( g_ColUserFrameBG );
	RECT rcUserFrame;

	const int nTextX = nX+4;
	const int nTextY1 = nY+4;
	const int nTextY2 = nTextY1+36;

	SetRect( &rcUserFrame, nX, nY, nX+nW, nY+nH );
	FillRect( hDC, &rcUserFrame, hBrush2 );

	if( pUser->GetUserImage() != NULL )
	{
		DrawImage( hDC, 
			pUser->GetUserImage(), 
			nX+((nW - 64) - 4), 
			nY+4, 
			64, 64 );
	}

	SetTextColor( hDC, g_ColText );
	SelectObject( hDC, g_hFontDesc );

	sprintf_s( buffer, 256, " %s ", pUser->Username().c_str() );
	TextOut( hDC, nTextX, nTextY1, buffer, strlen( buffer ) );

	sprintf_s( buffer, 256, " %d Points ", pUser->GetScore() );
	TextOut( hDC, nTextX, nTextY2, buffer, strlen( buffer ) );

	if( g_hardcoreModeActive )
	{
		COLORREF nLastColor = SetTextColor( hDC, g_ColWarning1 );
		COLORREF nLastColorBk = SetBkColor( hDC, g_ColWarning2 );

		sprintf_s( buffer, 256, " HARDCORE " );
		TextOut( hDC, nX+180, nY+70, buffer, strlen( buffer ) );

		SetTextColor( hDC, nLastColor );
		SetBkColor( hDC, nLastColorBk );
	}

	DeleteObject( hBrush2 );
}

const int* AchievementOverlay::GetActiveScrollOffset() const
{	
	switch( m_Pages[m_nPageStackPointer] )
	{
	case OP_ACHIEVEMENTS:
		return &m_nAchievementsScrollOffset;
	case OP_FRIENDS:
		return &m_nFriendsScrollOffset;
	case OP_MESSAGES:
		return &m_nMessagesScrollOffset;
	case OP_NEWS:
		return &m_nNewsScrollOffset;
	case OP_LEADERBOARDS:
		return &m_nLeaderboardScrollOffset;
	case OP_LEADERBOARD_EXAMINE:
	case OP_ACHIEVEMENT_EXAMINE:
		return 0;
	default:
		assert(0);
		return &m_nAchievementsScrollOffset;
	}
}

const int* AchievementOverlay::GetActiveSelectedItem() const
{
	switch( m_Pages[m_nPageStackPointer] )
	{
	case OP_ACHIEVEMENTS:
		return &m_nAchievementsSelectedItem;	//	?
	case OP_FRIENDS:
		return &m_nFriendsSelectedItem;
	case OP_MESSAGES:
		return &m_nMessagesSelectedItem;
	case OP_NEWS:
		return &m_nNewsSelectedItem;
	case OP_LEADERBOARDS:
		return &m_nLeaderboardSelectedItem;
	case OP_ACHIEVEMENT_EXAMINE:
	case OP_LEADERBOARD_EXAMINE:
		return 0;
	default:
		assert(0);
		return &m_nAchievementsSelectedItem;
	}
}

void AchievementOverlay::OnLoad_NewRom()
{
	m_nAchievementsScrollOffset = 0;
	if( IsActive() )
		Deactivate();
}

void AchievementOverlay::OnHTTP_UserPic( const char* sUsername )
{
 	
}

BOOL AchievementOverlay::IsActive()	
{ 
	return m_nTransitionState!=TS_OFF;
}

enum OverlayPage AchievementOverlay::CurrentPage()	
{ 
	return m_Pages[m_nPageStackPointer];
}

void AchievementOverlay::InitDirectX()
{
	if( g_RAMainWnd == NULL )
	{
		MessageBox( g_RAMainWnd, "InitDirectX failed: g_RAMainWnd invalid (check RA_Init has a valid HWND!)", "Error!", MB_OK );
		return;
	}

	//LPDIRECTDRAW lpDD_Init;
	//if (DirectDrawCreate(NULL, &lpDD_Init, NULL) != DD_OK)
	//{
	//	MessageBox( g_RAMainWnd, "DirectDrawCreate failed!", "Error!", MB_OK );
	//	return;
	//}
	//
	//if (lpDD_Init->QueryInterface(IID_IDirectDraw4, (LPVOID *) &m_lpDD) != DD_OK)
	//{
	//	MessageBox( g_RAMainWnd, "Error with QueryInterface!", "Error!", MB_OK );
	//	return;
	//}

	//lpDD_Init->Release();
	//lpDD_Init = NULL;

	//m_lpDD->SetCooperativeLevel( g_RAMainWnd, DDSCL_NORMAL );

	ResetDirectX();
}

void AchievementOverlay::ResetDirectX()
{
	//if( m_lpDD == NULL )
	//	return;

	//RECT rcTgtSize;
	//SetRect( &rcTgtSize, 0, 0, 640, 480 );

	//if( m_lpDDS_Overlay != NULL )
	//{
	//	m_lpDDS_Overlay->Release();
	//	m_lpDDS_Overlay = NULL;
	//}

	//DDSURFACEDESC2 ddsd;
	//memset(&ddsd, 0, sizeof(ddsd));
	//ddsd.dwSize = sizeof(ddsd);
	//ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	//
	//ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
	//ddsd.dwWidth = rcTgtSize.right - rcTgtSize.left;
	//ddsd.dwHeight = rcTgtSize.bottom - rcTgtSize.top;

	//HRESULT hr = m_lpDD->CreateSurface(&ddsd, &m_lpDDS_Overlay, NULL);
	//if( hr != DD_OK )
	//{
	//	assert(!"Cannot create overlay surface!");
	//	return;
	//}

}

void AchievementOverlay::Flip(HWND hWnd)
{
	//if( m_lpDDS_Overlay == NULL )
	//	return;

	//RECT rcDest;
	//GetWindowRect( g_RAMainWnd, &rcDest );
	//OffsetRect( &rcDest, -rcDest.left, -rcDest.top);

	//HDC hDC;
	//if( m_lpDDS_Overlay->GetDC( &hDC )== DD_OK )
	//{
	//	g_AchievementOverlay.Render( hDC, &rcDest );

	//	m_lpDDS_Overlay->ReleaseDC( hDC );
	//}
}

void AchievementOverlay::InstallNewsArticlesFromFile()
{
	m_LatestNews.clear();

	FILE* pf = NULL;
	fopen_s( &pf, RA_NEWS_FILENAME, "rb" );
	if( pf != NULL )
	{
		Document doc;
		doc.ParseStream( FileStream( pf ) );

		if( doc.HasMember("Success") && doc["Success"].GetBool() )
		{
			const Value& News = doc["News"];
			for( SizeType i = 0; i < News.Size(); ++i )
			{
				const Value& NextNewsArticle = News[i];
				
				NewsItem nNewsItem;
				nNewsItem.m_nID = NextNewsArticle["ID"].GetUint();
				nNewsItem.m_sTitle = NextNewsArticle["Title"].GetString();
				nNewsItem.m_sPayload = NextNewsArticle["Payload"].GetString();
				nNewsItem.m_sAuthor = NextNewsArticle["Author"].GetString();
				nNewsItem.m_sLink = NextNewsArticle["Link"].GetString();
				nNewsItem.m_sImage = NextNewsArticle["Image"].GetString();
				nNewsItem.m_nPostedAt = NextNewsArticle["TimePosted"].GetUint();
				
				tm destTime;
				localtime_s( &destTime, &nNewsItem.m_nPostedAt );

				char buffer[256];
				strftime( buffer, 256, "%b %d", &destTime );
				nNewsItem.m_sPostedAt = buffer;

				m_LatestNews.push_back( nNewsItem );
			}
		}

		fclose( pf );
	}
}

AchievementExamine::AchievementExamine() :
	m_pSelectedAchievement( NULL ),
	m_bHasData( FALSE ),
	m_nTotalWinners( 0 ),
	m_nPossibleWinners( 0 )
{
}

void AchievementExamine::Clear()
{
	m_pSelectedAchievement = NULL;
	m_CreatedDate.clear();
	m_LastModifiedDate.clear();
	m_bHasData = FALSE;
	m_nTotalWinners = 0;
	m_nPossibleWinners = 0;
	RecentWinners.clear();
}

void AchievementExamine::Initialize( const Achievement* pAch )
{
	Clear();
	m_pSelectedAchievement = pAch;

	if( pAch == NULL )
	{
		//	Do nothing.
	}
	else if( m_pSelectedAchievement->ID() == 0 )
	{
		//	Uncommitted/Exempt ID
		//	NB. Don't attempt to get anything
		m_bHasData = TRUE;
	}
	else
	{
		//	Go fetch data:
		m_CreatedDate = _TimeStampToString( pAch->CreatedDate() );
		m_LastModifiedDate = _TimeStampToString( pAch->ModifiedDate() );

		PostArgs args;
		args['a'] = std::to_string( m_pSelectedAchievement->ID() );
		RAWeb::CreateThreadedHTTPRequest( RequestAchievementInfo, args );
	}
}

void AchievementExamine::OnReceiveData( Document& doc )
{
	ASSERT( doc["Success"].GetBool() );
	const unsigned int nOffset = doc["Offset"].GetUint();
	const unsigned int nCount = doc["Count"].GetUint();
	const unsigned int nFriendsOnly = doc["FriendsOnly"].GetUint();
	const unsigned int nAchievementID = doc["AchievementID"].GetUint();
	const Value& ResponseData = doc["Response"];
	
	const unsigned int nEarnedBy = ResponseData["NumEarned"].GetUint();
	const unsigned int nTotalPlayers = ResponseData["TotalPlayers"].GetUint();
	const unsigned int nGameID = ResponseData["GameID"].GetUint();

	const Value& RecentWinnerData = ResponseData["RecentWinner"];
	ASSERT( RecentWinnerData.IsArray() );

	for( SizeType i = 0; i < RecentWinnerData.Size(); ++i )
	{
		const Value& NextWinner = RecentWinnerData[i];
		const std::string& sNextWinner = NextWinner["User"].GetString();
		const unsigned int nPoints = NextWinner["RAPoints"].GetUint();
		const time_t nDateAwarded = static_cast<time_t>( NextWinner["DateAwarded"].GetUint() );

		RecentWinners.push_back( AchievementExamine::RecentWinnerData( sNextWinner + " (" + std::to_string( nPoints ) + ")", _TimeStampToString( nDateAwarded ) ) );
	}

	m_bHasData = TRUE;
}

void LeaderboardExamine::Initialize( const unsigned int nLBIDIn )
{
	m_bHasData = FALSE;

	if( m_nLBID == nLBIDIn )
	{
		//	Same ID again: keep existing data
		m_bHasData = TRUE;
		return;
	}

	m_nLBID = nLBIDIn;

	unsigned int nOffset = 0;		//	TBD
	unsigned int nCount = 10;

	PostArgs args;
	args['i'] = std::to_string( m_nLBID );
	args['o'] = std::to_string( nOffset );
	args['c'] = std::to_string( nCount );

	RAWeb::CreateThreadedHTTPRequest( RequestLeaderboardInfo, args );
}

//static 
void LeaderboardExamine::OnReceiveData( const Document& doc )
{
	ASSERT( doc["Response"].IsArray() );
	const Value& DataReturned = doc["Response"];

	const Value& LBData = DataReturned["LeaderboardData"];
	ASSERT( LBData.IsArray() );
	
	unsigned int nLBID = LBData["LBID"].GetUint();
	unsigned int nGameID = LBData["GameID"].GetUint();
	const std::string& sGameTitle = LBData["GameTitle"].GetString();
	const std::string& sConsoleID = LBData["ConsoleID"].GetString();
	const std::string& sConsoleName = LBData["ConsoleName"].GetString();
	const std::string& sGameIcon = LBData["GameIcon"].GetString();
	//const std::string& sForumTopicID = LBData["ForumTopicID"].GetString();
	
	unsigned int nLowerIsBetter = LBData["LowerIsBetter"].GetUint();
	const std::string& sLBTitle = LBData["LBTitle"].GetString();
	const std::string& sLBDesc = LBData["LBDesc"].GetString();
	const std::string& sLBFormat = LBData["LBFormat"].GetString();
	const std::string& sLBMem = LBData["LBMem"].GetString();

	const Value& Entries = LBData["Entries"];
	
	RA_Leaderboard* pLB = g_LeaderboardManager.FindLB( nLBID );
	if( !pLB )
		return;

	for( SizeType i = 0; i < Entries.Size(); ++i )
	{
		const Value& NextLBData = Entries[i];
		const unsigned int nRank = NextLBData["Rank"].GetUint();
		const std::string& sUser = NextLBData["User"].GetString();
		const unsigned int nScore = NextLBData["Score"].GetUint();
		const unsigned int nDate = NextLBData["DateSubmitted"].GetUint();
		
		RA_LOG( "LB Entry: %d: %s earned %d at %d\n", nRank, sUser.c_str(), nScore, nDate );
		
		pLB->SubmitRankInfo( nRank, sUser.c_str(), nScore, nDate );
	}
}

//	Stubs for non-class based, indirect calling of these functions.
API int _RA_UpdateOverlay( ControllerInput* pInput, float fDTime, bool Full_Screen, bool Paused )
{
	return g_AchievementOverlay.Update( pInput, fDTime, Full_Screen, Paused );
}

API void _RA_RenderOverlay( HDC hDC, RECT* rcSize )
{
	g_AchievementOverlay.Render( hDC, rcSize );
}

API void _RA_InitDirectX()
{
	g_AchievementOverlay.InitDirectX();
}

API void _RA_OnPaint( HWND hWnd )
{
	g_AchievementOverlay.Flip( hWnd );
}