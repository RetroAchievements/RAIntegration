#include "RA_AchievementPopup.h"

#include "RA_Achievement.h"
#include "RA_Defs.h"
#include "RA_AchievementOverlay.h"
#include "RA_ImageFactory.h"

namespace
{
	const float POPUP_DIST_Y_TO_PCT = 0.856f;		//	Where on screen to end up
	const float POPUP_DIST_Y_FROM_PCT = 0.4f;		//	Amount of screens to travel
	const TCHAR* FONT_TO_USE = _T( "Tahoma" );
	
	const int FONT_SIZE_TITLE = 32;
	const int FONT_SIZE_SUBTITLE = 28;

	const float START_AT = 0.0f;
	const float APPEAR_AT = 0.8f;
	const float FADEOUT_AT = 4.2f;
	const float FINISH_AT = 5.0f;

	const TCHAR* MSG_SOUND[] =
	{
		_T( "./Overlay/login.wav" ),
		_T( "./Overlay/info.wav" ),
		_T( "./Overlay/unlock.wav" ),
		_T( "./Overlay/acherror.wav" ),
		_T( "./Overlay/lb.wav" ),
		_T( "./Overlay/lbcancel.wav" ),
		_T( "./Overlay/message.wav" ),
	};
	static_assert( SIZEOF_ARRAY( MSG_SOUND ) == NumMessageTypes, "Must match!" );
}

AchievementPopup::AchievementPopup() :
	m_fTimer( 0.0f )
{
}

void AchievementPopup::PlayAudio()
{
	ASSERT( MessagesPresent() );	//	ActiveMessage() dereferences!
	PlaySound( MSG_SOUND[ ActiveMessage().Type() ], NULL, SND_FILENAME|SND_ASYNC );
}

void AchievementPopup::AddMessage( const MessagePopup& msg )
{
	m_vMessages.push( msg );
	PlayAudio();
}

void AchievementPopup::Update( ControllerInput input, float fDelta, bool bFullScreen, bool bPaused )
{
	if( bPaused )
		fDelta = 0.0f;
	fDelta = RAClamp<float>( fDelta, 0.0f, 0.3f );	//	Limit this!
	if( m_vMessages.size() > 0 )
	{
		m_fTimer += fDelta;
		if( m_fTimer >= FINISH_AT )
		{
			m_vMessages.pop();
			m_fTimer = 0.0f;
		}
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

void AchievementPopup::Render( HDC hDC, RECT& rcDest )
{
	if( !MessagesPresent() )
		return;

	const int nPixelWidth = rcDest.right - rcDest.left;

	//SetBkColor( hDC, RGB( 0, 212, 0 ) );
	SetBkColor( hDC, COL_TEXT_HIGHLIGHT );
	SetTextColor( hDC, COL_POPUP );

	HFONT hFontTitle = CreateFont( FONT_SIZE_TITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
								   DEFAULT_PITCH, FONT_TO_USE );

	HFONT hFontDesc = CreateFont( FONT_SIZE_SUBTITLE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
								  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_CHARACTER_PRECIS, ANTIALIASED_QUALITY,/*NONANTIALIASED_QUALITY,*/
								  DEFAULT_PITCH, FONT_TO_USE );

	int nTitleX = 10;
	int nDescX = nTitleX + 2;

	const int nHeight = rcDest.bottom - rcDest.top;

	float fFadeInY = GetYOffsetPct() * ( POPUP_DIST_Y_FROM_PCT * static_cast<float>( nHeight ) );
	fFadeInY += ( POPUP_DIST_Y_TO_PCT * static_cast<float>( nHeight ) );

	const int nTitleY = static_cast<int>( fFadeInY );
	const int nDescY = nTitleY + 32;

	if( ActiveMessage().Type() == PopupAchievementUnlocked || ActiveMessage().Type() == PopupAchievementError )
	{
		DrawImage( hDC, ActiveMessage().Image(), nTitleX, nTitleY, 64, 64 );

		nTitleX += 64 + 4 + 2;	//	Negate the 2 from earlier!
		nDescX += 64 + 4;
	}
	else if( ActiveMessage().Type() == PopupLeaderboardInfo )
	{
		//	meh
	}

	const std::string sTitle = std::string( " " + ActiveMessage().Title() + " " );
	const std::string sSubTitle = std::string( " " + ActiveMessage().Subtitle() + " " );

	SelectObject( hDC, hFontTitle );
	TextOut( hDC, nTitleX, nTitleY, NativeStr( sTitle ).c_str(), sTitle.length() );
	SIZE szTitle = { 0, 0 };
	GetTextExtentPoint32( hDC, NativeStr( sTitle ).c_str(), sTitle.length(), &szTitle );

	SIZE szAchievement = { 0, 0 };
	if( ActiveMessage().Subtitle().length() > 0 )
	{
		SelectObject( hDC, hFontDesc );
		TextOut( hDC, nDescX, nDescY, NativeStr( sSubTitle ).c_str(), sSubTitle.length() );
		GetTextExtentPoint32( hDC, NativeStr( sSubTitle ).c_str(), sSubTitle.length(), &szAchievement );
	}

	HGDIOBJ hPen = CreatePen( PS_SOLID, 2, COL_POPUP_SHADOW );
	SelectObject( hDC, hPen );

	MoveToEx( hDC, nTitleX, nTitleY + szTitle.cy, NULL );
	LineTo( hDC, nTitleX + szTitle.cx, nTitleY + szTitle.cy );	//	right
	LineTo( hDC, nTitleX + szTitle.cx, nTitleY + 1 );			//	up

	if( ActiveMessage().Subtitle().length() > 0 )
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
	while( !m_vMessages.empty() )
		m_vMessages.pop();
}