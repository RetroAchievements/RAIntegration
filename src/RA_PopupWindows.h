#ifndef RA_POPUPWINDOWS_H
#define RA_POPUPWINDOWS_H
#pragma once

#include "RA_AchievementPopup.h"
#include "RA_Core.h"
#include "RA_LeaderboardPopup.h"

#include "ui\drawing\gdi\GDISurface.hh"

class PopupWindows
{
public:
    static void Update(_In_ float fDelta)
    {
        m_AchievementPopups.Update(fDelta);
        m_LeaderboardPopups.Update(fDelta);
    }

    static void Render(_In_ HDC hDC, const RECT* const restrict rcDest)
    {
        Expects(rcDest != nullptr);
        ra::ui::drawing::gdi::GDISurface pSurface(hDC, *rcDest);

        m_AchievementPopups.Render(pSurface);
        m_LeaderboardPopups.Render(pSurface);
    }

    static void Clear() noexcept { m_AchievementPopups.Clear(); }

    static AchievementPopup& AchievementPopups() noexcept { return m_AchievementPopups; }
    static LeaderboardPopup& LeaderboardPopups() noexcept { return m_LeaderboardPopups; }

private:
    static AchievementPopup m_AchievementPopups;
    static LeaderboardPopup m_LeaderboardPopups;
};

// Exposed to DLL
_EXTERN_C
GSL_SUPPRESS_CON3 API int _RA_UpdatePopups(_In_ _UNUSED ControllerInput* restrict, _In_ float fDTime,
                                           _In_ _UNUSED bool, _In_ bool Paused);
GSL_SUPPRESS_CON3 API int _RA_RenderPopups(_In_ HDC hDC, _In_ RECT* restrict rcSize);
_END_EXTERN_C

extern PopupWindows g_PopupWindows;

#endif // !RA_POPUPWINDOWS_H
