#ifndef RA_POPUPWINDOWS_H
#define RA_POPUPWINDOWS_H
#pragma once

#include "RA_AchievementPopup.h"
#include "RA_LeaderboardPopup.h"
#include "RA_Core.h"

class PopupWindows
{
public:
    static void Update(_In_ const ControllerInput* const __restrict pInput,
                       _In_ float fDelta,
                       _In_ bool bFullscreen,
                       _In_ bool bPaused)
    {
        m_AchievementPopups.Update(*pInput, fDelta, bFullscreen, bPaused);
        m_LeaderboardPopups.Update(*pInput, fDelta, bFullscreen, bPaused);
    }

    static void Render(_In_ HDC hDC, const RECT* const __restrict rcDest)
    {
        m_AchievementPopups.Render(hDC, *rcDest);
        m_LeaderboardPopups.Render(hDC, *rcDest);
    }

    static void Clear()
    {
        m_AchievementPopups.Clear();
    }

    static AchievementPopup& AchievementPopups() noexcept { return m_AchievementPopups; }
    static LeaderboardPopup& LeaderboardPopups() noexcept { return m_LeaderboardPopups; }

private:
    static AchievementPopup m_AchievementPopups;
    static LeaderboardPopup m_LeaderboardPopups;
};

// Exposed to DLL
_EXTERN_C
[[gsl::suppress(con.3)]]
API int _RA_UpdatePopups(_In_ ControllerInput* __restrict input, _In_ float fDTime,
                         _In_ bool Full_Screen, _In_ bool Paused);
[[gsl::suppress(con.3)]]
API int _RA_RenderPopups(_In_ HDC hDC, _In_ RECT* __restrict rcSize);
_END_EXTERN_C

extern PopupWindows g_PopupWindows;


#endif // !RA_POPUPWINDOWS_H
