#include "RA_PopupWindows.h"

PopupWindows g_PopupWindows;

//static 
AchievementPopup PopupWindows::m_AchievementPopups;
LeaderboardPopup PopupWindows::m_LeaderboardPopups;

// Stubs for non-class based, indirect calling of these functions.
_Use_decl_annotations_
API int _RA_UpdatePopups(ControllerInput* __restrict input, float fDTime, bool Full_Screen, bool Paused)
{
    PopupWindows::Update(input, fDTime, Full_Screen, Paused);
    return 0;
}

_Use_decl_annotations_
API int _RA_RenderPopups(HDC hDC, RECT* __restrict rcSize)
{
    if (!g_AchievementOverlay.IsFullyVisible())
        PopupWindows::Render(hDC, rcSize);

    return 0;
}
