#include "RA_PopupWindows.h"

PopupWindows g_PopupWindows;

//static 
ProgressPopup PopupWindows::m_ProgressPopups;
AchievementPopup PopupWindows::m_AchievementPopups;
LeaderboardPopup PopupWindows::m_LeaderboardPopups;

//	Stubs for non-class based, indirect calling of these functions.
API int _RA_UpdatePopups( ControllerInput* input, float fDTime, bool Full_Screen, bool Paused )
{
	PopupWindows::Update( input, fDTime, Full_Screen, Paused );
	return 0;
}

API int _RA_RenderPopups( HDC hDC, RECT* rcSize )
{
	PopupWindows::Render( hDC, rcSize );
	return 0;
}