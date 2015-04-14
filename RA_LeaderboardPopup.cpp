#include "RA_LeaderboardPopup.h"

#include <windows.h>
#include <stdio.h>

#include "RA_Achievement.h"
#include "RA_AchievementOverlay.h"
#include "RA_ImageFactory.h"
#include "RA_Leaderboard.h"

//	No emulator-specific code here please!

namespace
{
	const float SCOREBOARD_APPEAR_AT = 0.8f;
	const float SCOREBOARD_FADEOUT_AT = 6.2f;
	const float SCOREBOARD_FINISH_AT = 7.0f;

	const char* FONT_TO_USE = "Tahoma";

	const COLORREF g_ColBG = RGB( 32, 32, 32 );
	
	const int FONT_SIZE_TITLE = 28;
	const int FONT_SIZE_SUBTITLE = 22;
	const int FONT_SIZE_TEXT = 16;
}


LeaderboardPopup::LeaderboardPopup()
{
	Reset();
}

void LeaderboardPopup::ShowScoreboard( LeaderboardID nID )
{
	m_vScoreboardQueue.push( nID );

	if( m_fScoreboardShowTimer == SCOREBOARD_FINISH_AT )
	{
		m_fScoreboardShowTimer = 0.0f;
		m_nState = State_ShowingScoreboard;
	}
}

void LeaderboardPopup::Reset()
{
	m_fScoreboardShowTimer = SCOREBOARD_FINISH_AT;
	while( !m_vScoreboardQueue.empty() )
		m_vScoreboardQueue.pop();

	m_vActiveLBIDs.clear();
	m_nState = State_ShowingProgress;
}

void LeaderboardPopup::Update( ControllerInput input, float fDelta, BOOL bFullScreen, BOOL bPaused )
{
	if( !g_bLeaderboardsActive )	//	If not, simply ignore them.
		return;

	if( bPaused )
		fDelta = 0.0f;

	if( m_fScoreboardShowTimer >= SCOREBOARD_FINISH_AT )
	{
		//	No longer showing the scoreboard
		if( !m_vScoreboardQueue.empty() )
		{
			m_vScoreboardQueue.pop();

			if( !m_vScoreboardQueue.empty() )
			{
				//	Still not empty: restart timer
				m_fScoreboardShowTimer = 0.0f;	//	Show next scoreboard
			}
			else
			{
				m_fScoreboardShowTimer = SCOREBOARD_FINISH_AT;
				m_nState = State_ShowingProgress;
			}
		}
		else
		{
			m_fScoreboardShowTimer = SCOREBOARD_FINISH_AT;
			m_nState = State_ShowingProgress;
		}
	}
	else
	{
		m_fScoreboardShowTimer += fDelta;
	}
}

BOOL LeaderboardPopup::Activate( unsigned int nLBID )
{
	std::vector<unsigned int>::iterator iter = m_vActiveLBIDs.begin();
	while( iter != m_vActiveLBIDs.end() )
	{
		if( (*iter) == nLBID )
			return FALSE;
		iter++;
	}

	m_vActiveLBIDs.push_back( nLBID );
	return TRUE;
}

BOOL LeaderboardPopup::Deactivate( unsigned int nLBID )
{
	std::vector<unsigned int>::iterator iter = m_vActiveLBIDs.begin();
	while( iter != m_vActiveLBIDs.end() )
	{
		if( (*iter) == nLBID )
		{
			m_vActiveLBIDs.erase( iter );
			return TRUE;
		}
		iter++;
	}

	RA_LOG( "Could not deactivate leaderboard %d", nLBID );

	return FALSE;
}

float LeaderboardPopup::GetOffsetPct() const
{
	float fVal = 0.0f;

	if( m_fScoreboardShowTimer < SCOREBOARD_APPEAR_AT )
	{
		//	Fading in.
		float fDelta = (SCOREBOARD_APPEAR_AT - m_fScoreboardShowTimer);

		fDelta *= fDelta;	//	Quadratic

		fVal = fDelta;
	}
	else if( m_fScoreboardShowTimer < (SCOREBOARD_FADEOUT_AT) )
	{
		//	Faded in - held
		fVal = 0.0f;
	}
	else if( m_fScoreboardShowTimer < (SCOREBOARD_FINISH_AT) )
	{
		//	Fading out
		float fDelta = ( SCOREBOARD_FADEOUT_AT - m_fScoreboardShowTimer );

		fDelta *= fDelta;	//	Quadratic

		fVal = ( fDelta );
	}
	else
	{
		//	Finished!
		fVal = 1.0f;
	}

	return fVal;
}

void LeaderboardPopup::Render( HDC hDC, RECT& rcDest )
{
	if( !g_bLeaderboardsActive )	//	If not, simply ignore them.
		return;

	SetBkColor( hDC, COL_TEXT_HIGHLIGHT );
	SetTextColor( hDC, COL_POPUP );

	HFONT hFontTitle = CreateFont( FONT_SIZE_TITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
								   DEFAULT_PITCH, Widen( FONT_TO_USE ).c_str() );

	HFONT hFontDesc = CreateFont( FONT_SIZE_SUBTITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
								  DEFAULT_PITCH, Widen( FONT_TO_USE ).c_str() );

	HFONT hFontText = CreateFont( FONT_SIZE_TEXT, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
								  DEFAULT_PITCH, Widen( FONT_TO_USE ).c_str() );


	const int nWidth = rcDest.right - rcDest.left;
	const int nHeight = rcDest.bottom - rcDest.top;

	//float fOffscreenAmount = ( GetOffsetPct() * ( POPUP_DIST_FROM_PCT * (float)nWidth ) );
	float fOffscreenAmount = ( GetOffsetPct() * 600 );
	//float fFadeOffs = (POPUP_DIST_TO_PCT * (float)nWidth ) + fOffscreenAmount;
	float fFadeOffs = (nWidth-300) + fOffscreenAmount;

	int nScoreboardX = (int)fFadeOffs;
	int nScoreboardY = (int)nHeight-200;

	int nRightLim = (int)( (nWidth-8) + fOffscreenAmount );

	//if( GetMessageType() == 1 )
	//{
	//	DrawImage( hDC, GetImage(), nTitleX, nTitleY, 64, 64 );

	//	nTitleX += 64+4+2;	//	Negate the 2 from earlier!
	//	nDescX += 64+4;
	//}


	HGDIOBJ hPen = CreatePen( PS_SOLID, 2, RGB( 0, 0, 0 ) );

	HBRUSH hBrushBG = CreateSolidBrush( g_ColBG );

	RECT rcScoreboardFrame;
	SetRect( &rcScoreboardFrame, nScoreboardX, nScoreboardY, nRightLim, nHeight-8 );
	InflateRect( &rcScoreboardFrame, 2, 2 );
	FillRect( hDC, &rcScoreboardFrame, hBrushBG );

	HGDIOBJ hOld = SelectObject( hDC, hFontDesc );

	switch( m_nState )
	{
	case State_ShowingProgress:
	{
		int nProgressYOffs = 0;
		std::vector<unsigned int>::const_iterator iter = m_vActiveLBIDs.begin();
		while( iter != m_vActiveLBIDs.end() )
		{
			const RA_Leaderboard* pLB = g_LeaderboardManager.FindLB( *iter );
			if( pLB != nullptr )
			{
				//	Show current progress:
				std::string sScoreSoFar = std::string( " " ) + pLB->FormatScore( static_cast<int>( pLB->GetCurrentValueProgress() ) ) + std::string( " " );

				SIZE szProgress;
				GetTextExtentPoint32( hDC, Widen( sScoreSoFar ).c_str(), sScoreSoFar.length(), &szProgress );

				HGDIOBJ hBkup = SelectObject( hDC, hPen );

				MoveToEx( hDC, nWidth - 8, nHeight - 8 - szProgress.cy + nProgressYOffs, nullptr );
				LineTo( hDC, nWidth - 8, nHeight - 8 + nProgressYOffs );							//	down
				LineTo( hDC, nWidth - 8 - szProgress.cx, nHeight - 8 + nProgressYOffs );			//	left

				RECT rcProgress;
				SetRect( &rcProgress, 0, 0, nWidth - 8, nHeight - 8 + nProgressYOffs );
				DrawText( hDC, Widen( sScoreSoFar ).c_str(), sScoreSoFar.length(), &rcProgress, DT_BOTTOM | DT_RIGHT | DT_SINGLELINE );

				SelectObject( hDC, hBkup );
				nProgressYOffs -= 26;
			}

			iter++;
		}
	}
		break;

	case State_ShowingScoreboard:
		{
			const RA_Leaderboard* pLB = g_LeaderboardManager.FindLB( m_vScoreboardQueue.front() );
			if( pLB != nullptr )
			{
				char buffer[ 1024 ];
				sprintf_s( buffer, 1024, " Results: %s ", pLB->Title().c_str() );
				RECT rcTitle = { nScoreboardX + 2, nScoreboardY + 2, nRightLim - 2, nHeight - 8 };
				DrawText( hDC, Widen( buffer ).c_str(), strlen( buffer ), &rcTitle, DT_TOP | DT_LEFT | DT_SINGLELINE );

				//	Show scoreboard
				RECT rcScoreboard = { nScoreboardX + 2, nScoreboardY + 32, nRightLim - 2, nHeight - 16 };
				for( size_t i = 0; i < pLB->GetRankInfoCount(); ++i )
				{
					const LB_Entry& lbInfo = pLB->GetRankInfo( i );

					if( lbInfo.m_sUsername.compare( RAUsers::LocalUser().Username() ) == 0 )
					{
						SetBkMode( hDC, OPAQUE );
						SetTextColor( hDC, COL_POPUP );
					}
					else
					{
						SetBkMode( hDC, TRANSPARENT );
						SetTextColor( hDC, COL_TEXT_HIGHLIGHT );
					}

					char buffer[ 1024 ];
					sprintf_s( buffer, 1024, " %d %s ", lbInfo.m_nRank, lbInfo.m_sUsername.c_str() );
					DrawText( hDC, Widen( buffer ).c_str(), strlen( buffer ), &rcScoreboard, DT_TOP | DT_LEFT | DT_SINGLELINE );

					std::string sScore( " " + pLB->FormatScore( lbInfo.m_nScore ) + " " );
					DrawText( hDC, Widen( sScore ).c_str(), sScore.length(), &rcScoreboard, DT_TOP | DT_RIGHT | DT_SINGLELINE );

					rcScoreboard.top += 24;

					//	If we're about to draw the local, outranked player, offset a little more
					if( i == 5 )
						rcScoreboard.top += 4;
				}
			}
			
			//	Restore
			//SetBkMode( hDC, nOldBkMode );
		}
		break;

	default:
		break;
	}

	//	Restore old obj
	SelectObject( hDC, hOld );

	DeleteObject( hBrushBG );
	DeleteObject( hPen );
	DeleteObject( hFontTitle );
	DeleteObject( hFontDesc );
	DeleteObject( hFontText );
}
