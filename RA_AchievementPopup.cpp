#include "RA_AchievementPopup.h"

#include "RA_Achievement.h"
#include <windows.h>
#include <stdio.h>

#include "RA_Defs.h"
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

AchievementPopup::AchievementPopup() :
	m_fTimer( 0.0f )
{
}

void AchievementPopup::PlayAudio()
{
	if( MessagesPresent() )
	{
		switch( ActiveMessage().Type() )
		{
		case PopupAchievementUnlocked:
			PlaySound( "./Overlay/unlock.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case PopupAchievementError:
			PlaySound( "./Overlay/acherror.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case PopupLeaderboardInfo:
			PlaySound( "./Overlay/lb.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case PopupLeaderboardCancel:
			PlaySound( "./Overlay/lbcancel.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case PopupLogin:
			PlaySound( "./Overlay/login.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		case PopupInfo:
			PlaySound( "./Overlay/info.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		default:
			PlaySound( "./Overlay/message.wav", NULL, SND_FILENAME|SND_ASYNC );
			break;
		}
	}
}

void AchievementPopup::AddMessage( const MessagePopup& msg )
{
	//	Add to the first available slot.
	bool bActive = MessagesPresent();
	m_vMessages.push( msg );

	if( !bActive )
		PlayAudio();
}

void AchievementPopup::Update( ControllerInput input, float fDelta, bool bFullScreen, bool bPaused )
{
	if( bPaused )
		fDelta = 0.0f;

	fDelta = RA::Clamp<float>( fDelta, 0.0f, 0.3f );	//	Limit this!

	//if( m_bSuppressDeltaUpdate )
	//{
	//	m_bSuppressDeltaUpdate = false;
	//	return;
	//}

	if( m_vMessages.size() > 0 )
		m_fTimer += fDelta;

	if( ( m_vMessages.size() > 0 ) && ( m_fTimer >= FINISH_AT ) )
	{
		m_vMessages.pop();
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

void AchievementPopup::Render( HDC hDC, RECT& rcDest )
{
	if( !MessagesPresent() )
		return;

	const MessagePopup& msg = ActiveMessage();

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
	int nDescX = nTitleX + 2;

	const int nHeight = rcDest.bottom - rcDest.top;

	float fFadeInY = GetYOffsetPct() * ( POPUP_DIST_Y_FROM_PCT * (float)nHeight );
	fFadeInY += ( POPUP_DIST_Y_TO_PCT * (float)nHeight );

	int nTitleY = (int)fFadeInY;
	int nDescY = nTitleY + 32;

	if( msg.Type() == PopupAchievementUnlocked || msg.Type() == PopupAchievementError )
	{
		DrawImage( hDC, msg.Image(), nTitleX, nTitleY, 64, 64 );

		nTitleX += 64 + 4 + 2;	//	Negate the 2 from earlier!
		nDescX += 64 + 4;
	}
	else if( msg.Type() == PopupLeaderboardInfo )
	{
		//	meh
	}

	SIZE szTitle = { 0, 0 }, szAchievement = { 0, 0 };

	SelectObject( hDC, hFontTitle );
	TextOut( hDC, nTitleX, nTitleY, ActiveMessage().Title().c_str(), strlen( ActiveMessage().Title().c_str() ) );
	GetTextExtentPoint32( hDC, msg.Title().c_str(), strlen( msg.Title().c_str() ), &szTitle );
	
	SelectObject( hDC, hFontDesc );
	TextOut( hDC, nDescX, nDescY, msg.Subtitle().c_str(), strlen( msg.Subtitle().c_str() ) );
	GetTextExtentPoint32( hDC, msg.Subtitle().c_str(), strlen( msg.Subtitle().c_str() ), &szAchievement );

	HGDIOBJ hPen = CreatePen( PS_SOLID, 2, g_ColPopupShadow );
	SelectObject( hDC, hPen );

	MoveToEx( hDC, nTitleX, nTitleY + szTitle.cy, NULL );
	LineTo( hDC, nTitleX + szTitle.cx, nTitleY + szTitle.cy );	//	right
	LineTo( hDC, nTitleX + szTitle.cx, nTitleY + 1 );			//	up

	if( msg.Subtitle().length() > 0 )
	{
		MoveToEx( hDC, nDescX, nDescY + szAchievement.cy, NULL );
		LineTo( hDC, nDescX + szAchievement.cx, nDescY + szAchievement.cy );
		LineTo( hDC, nDescX + szAchievement.cx, nDescY + 1 );
	}

	DeleteObject( hPen );
	DeleteObject( hFontTitle );
	DeleteObject( hFontDesc );
}

void AchievementPopup::Clear()
{
	while( m_vMessages.size() > 0 )
		m_vMessages.pop();
}