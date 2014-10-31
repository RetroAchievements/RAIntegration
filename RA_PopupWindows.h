#pragma once

#include "RA_PopupWindows.h"

#include "RA_ProgressPopup.h"
#include "RA_AchievementPopup.h"
#include "RA_LeaderboardPopup.h"
#include "RA_Core.h"


class PopupWindows
{
public:
	static void Update( ControllerInput* pInput, float fDelta, bool bFullscreen, bool bPaused )
	{
		m_ProgressPopups.Update( *pInput, fDelta, bFullscreen, bPaused );
		m_AchievementPopups.Update( *pInput, fDelta, bFullscreen, bPaused );
		m_LeaderboardPopups.Update( *pInput, fDelta, bFullscreen, bPaused );
	}

	static void Render( HDC hDC, RECT* rcDest )
	{
		m_ProgressPopups.Render( hDC, *rcDest );
		m_AchievementPopups.Render( hDC, *rcDest );
		m_LeaderboardPopups.Render( hDC, *rcDest );
	}

	static void Clear()
	{
		m_ProgressPopups.Clear();
		m_AchievementPopups.Clear();
	}

	static ProgressPopup& ProgressPopups()			{ return m_ProgressPopups; }
	static AchievementPopup& AchievementPopups()	{ return m_AchievementPopups; }
	static LeaderboardPopup& LeaderboardPopups()	{ return m_LeaderboardPopups; }

private:
	static ProgressPopup m_ProgressPopups;
	static AchievementPopup m_AchievementPopups;
	static LeaderboardPopup m_LeaderboardPopups;
};

//	Exposed to DLL
extern "C"
{
API extern int _RA_UpdatePopups( ControllerInput* input, float fDTime, bool Full_Screen, bool Paused );
API extern int _RA_RenderPopups( HDC hDC, RECT* rcSize );
}

extern PopupWindows g_PopupWindows;
