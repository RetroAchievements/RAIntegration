#include "RA_PopupWindows.h"

PopupWindows g_PopupWindows;

//static 
AchievementPopup PopupWindows::m_AchievementPopups;
LeaderboardPopup PopupWindows::m_LeaderboardPopups;

// Stubs for non-class based, indirect calling of these functions.
_Use_decl_annotations_
GSL_SUPPRESS_CON3
API int _RA_UpdatePopups(_UNUSED ControllerInput* restrict, float fDTime, _UNUSED bool, bool Paused)
{
    if (Paused)
        fDTime = 0.0F;

    PopupWindows::Update(fDTime);
    return 0;
}

_Use_decl_annotations_
GSL_SUPPRESS_CON3
API int _RA_RenderPopups(HDC hDC, RECT* restrict rcSize)
{
    if (!g_AchievementOverlay.IsFullyVisible())
        PopupWindows::Render(hDC, rcSize);

    return 0;
}


