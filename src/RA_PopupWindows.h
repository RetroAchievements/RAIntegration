#ifndef RA_POPUPWINDOWS_H
#define RA_POPUPWINDOWS_H
#pragma once

#include "RA_AchievementPopup.h"
#include "RA_LeaderboardPopup.h"
#include "RA_Core.h"

class PopupWindows
{
public:
    static void Update(ControllerInput* pInput, float fDelta, bool bFullscreen, bool bPaused)
    {
        m_AchievementPopups.Update(*pInput, fDelta, bFullscreen, bPaused);
        m_LeaderboardPopups.Update(*pInput, fDelta, bFullscreen, bPaused);
    }

    static void Render(HDC hDC, RECT* rcDest)
    {
        m_AchievementPopups.Render(hDC, *rcDest);
        m_LeaderboardPopups.Render(hDC, *rcDest);
    }

    static void Clear()
    {
        m_AchievementPopups.Clear();
    }

    static AchievementPopup& AchievementPopups() { return m_AchievementPopups; }
    static LeaderboardPopup& LeaderboardPopups() { return m_LeaderboardPopups; }

private:
    static AchievementPopup m_AchievementPopups;
    static LeaderboardPopup m_LeaderboardPopups;
};

//	Exposed to DLL
extern "C"
{
    API extern int _RA_UpdatePopups(ControllerInput* input, float fDTime, bool Full_Screen, bool Paused);
    API extern int _RA_RenderPopups(HDC hDC, RECT* rcSize);
}

extern PopupWindows g_PopupWindows;


#endif // !RA_POPUPWINDOWS_H
