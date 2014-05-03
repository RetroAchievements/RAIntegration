#include "RA_AchievementPopup.h"

#include "RA_Achievement.h"
#include <windows.h>
#include <stdio.h>

#include "RA_AchievementOverlay.h"
#include "RA_ImageFactory.h"

//	No game-specific code here please!

//START_AT					(0.0f)
#define APPEAR_AT			(0.8f)
#define FADEOUT_AT			(4.2f)
#define FINISH_AT			(5.0f)

//	Where on screen to end up
#define POPUP_DIST_Y_TO_PCT		(0.856f)
//	Amount of screens to travel
#define POPUP_DIST_Y_FROM_PCT	(0.4f)

#define FONT_TO_USE "Tahoma"


//AchievementPopup g_PopupWindow;

AchievementPopup::AchievementPopup()
{
	m_fTimer = 0.0f;
	m_bSuppressDeltaUpdate = false;
}

void AchievementPopup::PlayAudio()
{
	if( IsActive() )
	{
		switch( GetMessageType() )
		{
		case MSG_ACHIEVEMENT_UNLOCKED:
			PlaySound( "./Overlay/unlock.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case MSG_ACHIEVEMENT_ERROR:
			PlaySound( "./Overlay/acherror.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case MSG_LEADERBOARD_INFO:
			PlaySound( "./Overlay/lb.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case MSG_LEADERBOARD_CANCEL:
			PlaySound( "./Overlay/lbcancel.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case MSG_LOGIN:
			PlaySound( "./Overlay/login.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case MSG_INFO:
			PlaySound( "./Overlay/info.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		default:
			PlaySound( "./Overlay/message.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		}
	}
}

void AchievementPopup::AddMessage( const char* sTitle, const char* sDesc, int nMessageType, HBITMAP hImage )
{
	//	Add to the first available slot.
	MessagePopup nNewMsg;
	strcpy_s( nNewMsg.m_sMessageTitle, 1024, sTitle );
	strcpy_s( nNewMsg.m_sMessageDesc, 1024, sDesc );
	nNewMsg.m_nMessageType = nMessageType;
	nNewMsg.m_hMessageImage = hImage;

	bool bActive = IsActive();
	m_vMessages.push( nNewMsg );

	if( !bActive )
		PlayAudio();
}

void AchievementPopup::NextMessage()
{
	m_vMessages.pop();
}

void AchievementPopup::Update( ControllerInput input, float fDelta, bool bFullScreen, bool bPaused )
{
	if( bPaused )
		fDelta = 0.0f;

	if( m_bSuppressDeltaUpdate )
	{
		m_bSuppressDeltaUpdate = false;
		return;
	}

	if( m_vMessages.size() > 0 )
		m_fTimer += fDelta;

	if( ( m_vMessages.size() > 0 ) && ( m_fTimer >= FINISH_AT ) )
	{
		NextMessage();
		PlayAudio();
		m_fTimer = 0.0f;
	}
}

float AchievementPopup::GetYOffsetPct() const
{
	float fVal = 0.0f;

	if( m_fTimer < APPEAR_AT )
	{
		//	Fading in.
		float fDelta = (APPEAR_AT - m_fTimer);

		fDelta *= fDelta;	//	Quadratic

		fVal = fDelta;
	}
	else if( m_fTimer < (FADEOUT_AT) )
	{
		//	Faded in - held
		fVal = 0.0f;
	}
	else if( m_fTimer < (FINISH_AT) )
	{
		//	Fading out
		float fDelta = ( FADEOUT_AT - m_fTimer );

		fDelta *= fDelta;	//	Quardratic

		fVal = ( fDelta );
	}
	else
	{
		//	Finished!
		fVal = 1.0f;
	}

	return fVal;
}

void AchievementPopup::Render( HDC hDC, RECT& rcDest )
{
	if( !IsActive() )
		return;

	const int nPixelWidth = rcDest.right - rcDest.left;

	const int nFontSize1 = 32;
	const int nFontSize2 = 28;

	//SetBkColor( hDC, RGB( 0, 212, 0 ) );
	SetBkColor( hDC, g_ColTextHighlight );
	SetTextColor( hDC, g_ColPopupText );

	HFONT hFontTitle = CreateFont( nFontSize1, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
		DEFAULT_PITCH, TEXT(FONT_TO_USE) );

	HFONT hFontDesc = CreateFont( nFontSize2, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
		DEFAULT_PITCH, TEXT(FONT_TO_USE) );


	int nTitleX = 10;
	int nDescX = nTitleX+2;

	const int nHeight = rcDest.bottom - rcDest.top;

	float fFadeInY = GetYOffsetPct() * ( POPUP_DIST_Y_FROM_PCT * (float)nHeight );
	fFadeInY += (POPUP_DIST_Y_TO_PCT * (float)nHeight );

	int nTitleY = (int)fFadeInY;
	int nDescY = nTitleY + 32;

	if( GetMessageType() == MSG_ACHIEVEMENT_UNLOCKED || GetMessageType() == MSG_ACHIEVEMENT_ERROR )
	{
		DrawImage( hDC, GetImage(), nTitleX, nTitleY, 64, 64 );

		nTitleX += 64+4+2;	//	Negate the 2 from earlier!
		nDescX += 64+4;
	}
	else if( GetMessageType() == MSG_LEADERBOARD_INFO )
	{
		//	meh
	}

	SIZE szTitle, szAchievement;

	SelectObject( hDC, hFontTitle );
	TextOut( hDC, nTitleX, nTitleY, (LPCSTR)GetTitle(), strlen( GetTitle() ) );
	GetTextExtentPoint32( hDC, GetTitle(), strlen( GetTitle() ), &szTitle );
	
	//SetBkColor( hDC, g_ColPopupBG );

	SelectObject( hDC, hFontDesc );
	TextOut( hDC, nDescX, nDescY, (LPCSTR)GetDesc(), strlen( GetDesc() ) );
	GetTextExtentPoint32( hDC, GetDesc(), strlen( GetDesc() ), &szAchievement );

	HGDIOBJ hPen = CreatePen( PS_SOLID, 2, g_ColPopupShadow );
	SelectObject( hDC, hPen );

	MoveToEx( hDC, nTitleX, nTitleY+szTitle.cy, NULL );
	LineTo( hDC, nTitleX+szTitle.cx, nTitleY+szTitle.cy );	//	right
	LineTo( hDC, nTitleX+szTitle.cx, nTitleY+1 );			//	up

	if( GetDesc()[0] != '\0' )
	{
		MoveToEx( hDC, nDescX, nDescY+szAchievement.cy, NULL );
		LineTo( hDC, nDescX+szAchievement.cx, nDescY+szAchievement.cy );
		LineTo( hDC, nDescX+szAchievement.cx, nDescY+1 );
	}

	DeleteObject(hPen);
	DeleteObject(hFontTitle);
	DeleteObject(hFontDesc);
}

void AchievementPopup::Clear()
{
	for( int i = 0; i < OVERLAY_MESSAGE_QUEUE_SIZE; ++i )
		NextMessage();
}