#include "RA_ProgressPopup.h"

#include <stdio.h>

#include "RA_Achievement.h"
#include "RA_AchievementOverlay.h"
#include "RA_ImageFactory.h"

namespace
{
	const int FONT_SIZE_MAIN = 32;
	const int FONT_SIZE_SUBTITLE = 28;

	const float APPEAR_AT = 0.8f;
	const float FADEOUT_AT = 4.2f;
	const float FINISH_AT = 5.0f;

	const float POPUP_DIST_Y_TO_PCT = 0.856f;	//	Where on screen to end up
	const float POPUP_DIST_Y_FROM_PCT = 0.4f;	//	Amount of screens to travel
	const char* FONT_TO_USE = "Tahoma";
}

//ProgressPopup g_ProgressPopup;

ProgressPopup::ProgressPopup()
{
	m_fTimer = 0.0f;
	m_bSuppressDeltaUpdate = false;
	for( size_t i = 0; i < OVERLAY_MESSAGE_QUEUE_SIZE; ++i )
	{
		memset( m_sMessageTitleQueue[ i ], '\0', 1024 );
		memset( m_sMessageDescQueue[ i ], '\0', 1024 );
		m_nMessageType[ i ] = 0;
		m_hMessageImage[ i ] = NULL;
	}
}

void ProgressPopup::AddMessage( const char* sTitle, const char* sDesc, int nMessageType, HBITMAP hImage )
{
	//	Add to the first available slot.
	for( size_t i = 0; i < OVERLAY_MESSAGE_QUEUE_SIZE; ++i )
	{
		if( m_sMessageTitleQueue[ i ][ 0 ] == '\0' )
		{
			sprintf_s( m_sMessageTitleQueue[ i ], 1024, "%s", sTitle );

			if( sDesc != NULL )
				sprintf_s( m_sMessageDescQueue[ i ], 1024, "%s", sDesc );
			else
				sprintf_s( m_sMessageDescQueue[ i ], 1024, "%s", "" );

			m_hMessageImage[ i ] = hImage;
			m_nMessageType[ i ] = nMessageType;
			break;
		}
	}
}

void ProgressPopup::NextMessage()
{
	//	Mem-move. Copy the lower 4 elements up a notch, and invalidate the fifth one.
	for( size_t i = 0; i < OVERLAY_MESSAGE_QUEUE_SIZE - 1; ++i )
	{
		memcpy( (void*)m_sMessageTitleQueue[ i ], (void*)m_sMessageTitleQueue[ i + 1 ], 1024 );
		memcpy( (void*)m_sMessageDescQueue[ i ], (void*)m_sMessageDescQueue[ i + 1 ], 1024 );

		m_hMessageImage[ i ] = m_hMessageImage[ i + 1 ];
		m_nMessageType[ i ] = m_nMessageType[ i + 1 ];
	}

	//	Invalidate the outer message
	memset( m_sMessageTitleQueue[ OVERLAY_MESSAGE_QUEUE_SIZE - 1 ], 0, 1024 );
	memset( m_sMessageDescQueue[ OVERLAY_MESSAGE_QUEUE_SIZE - 1 ], 0, 1024 );
	m_nMessageType[ OVERLAY_MESSAGE_QUEUE_SIZE - 1 ] = 0;
	m_hMessageImage[ OVERLAY_MESSAGE_QUEUE_SIZE - 1 ] = NULL;
}

void ProgressPopup::Update( ControllerInput input, float fDelta, BOOL bFullScreen, BOOL bPaused )
{
	if( !IsActive() )
		return;

	if( bPaused )
		fDelta = 0.0f;

	if( m_bSuppressDeltaUpdate )
	{
		m_bSuppressDeltaUpdate = false;
		return;
	}

	m_fTimer += fDelta;

	if( m_fTimer >= FINISH_AT )
	{
		if( m_sMessageTitleQueue[ 0 ][ 0 ] != '\0' )
		{
			NextMessage();
		}
		m_fTimer = 0.0f;
	}
}

float ProgressPopup::GetYOffsetPct() const
{
	float fVal = 0.0f;

	if( m_fTimer < APPEAR_AT )
	{
		//	Fading in.
		float fDelta = (APPEAR_AT - m_fTimer);

		fDelta *= fDelta;	//	Quadratic

		fVal = fDelta;
	}
	else if( m_fTimer < FADEOUT_AT )
	{
		//	Faded in - held
		fVal = 0.0f;
	}
	else if( m_fTimer < FINISH_AT )
	{
		//	Fading out
		float fDelta = ( FADEOUT_AT - m_fTimer );

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

void ProgressPopup::Render( HDC hDC, RECT& rcDest )
{
	if( !IsActive() )
		return;

	SetBkColor( hDC, RGB( 0, 212, 0 ) );
	SetTextColor( hDC, RGB( 0, 40, 0 ) );

	HFONT hFontTitle = CreateFont( FONT_SIZE_MAIN, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
								   DEFAULT_PITCH, Widen( FONT_TO_USE ).c_str() );

	HFONT hFontDesc = CreateFont( FONT_SIZE_SUBTITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
								  DEFAULT_PITCH, Widen( FONT_TO_USE ).c_str() );


	int nTitleX = 10;
	int nDescX = nTitleX + 2;

	const int nHeight = rcDest.bottom - rcDest.top;

	float fFadeInY = GetYOffsetPct() * ( POPUP_DIST_Y_FROM_PCT * (float)nHeight );
	fFadeInY += ( POPUP_DIST_Y_TO_PCT * (float)nHeight );

	int nTitleY = (int)fFadeInY;
	int nDescY = nTitleY + 32;

	if( GetMessageType() == 1 )
	{
		DrawImage( hDC, GetImage(), nTitleX, nTitleY, 64, 64 );

		nTitleX += 64 + 4 + 2;	//	Negate the 2 from earlier!
		nDescX += 64 + 4;
	}

	SIZE szTitle, szAchievement;

	SelectObject( hDC, hFontTitle );
	TextOut( hDC, nTitleX, nTitleY, Widen( GetTitle() ).c_str(), strlen( GetTitle() ) );
	GetTextExtentPoint32( hDC, Widen( GetTitle() ).c_str(), strlen( GetTitle() ), &szTitle );

	SelectObject( hDC, hFontDesc );
	TextOut( hDC, nDescX, nDescY, Widen( GetDesc() ).c_str(), strlen( GetDesc() ) );
	GetTextExtentPoint32( hDC, Widen( GetDesc() ).c_str(), strlen( GetDesc() ), &szAchievement );

	HGDIOBJ hPen = CreatePen( PS_SOLID, 2, RGB( 0, 0, 0 ) );
	SelectObject( hDC, hPen );

	MoveToEx( hDC, nTitleX, nTitleY + szTitle.cy, NULL );
	LineTo( hDC, nTitleX + szTitle.cx, nTitleY + szTitle.cy );	//	right
	LineTo( hDC, nTitleX + szTitle.cx, nTitleY + 1 );			//	up

	if( GetDesc()[ 0 ] != '\0' )
	{
		MoveToEx( hDC, nDescX, nDescY + szAchievement.cy, NULL );
		LineTo( hDC, nDescX + szAchievement.cx, nDescY + szAchievement.cy );
		LineTo( hDC, nDescX + szAchievement.cx, nDescY + 1 );
	}

	DeleteObject( hPen );
	DeleteObject( hFontTitle );
	DeleteObject( hFontDesc );
}

void ProgressPopup::Clear()
{
	for( int i = 0; i < OVERLAY_MESSAGE_QUEUE_SIZE; ++i )
		NextMessage();
}