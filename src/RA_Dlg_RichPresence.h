#ifndef RA_DLG_RICHPRESENCE_H
#define RA_DLG_RICHPRESENCE_H
#pragma once

#ifndef PCH_H
#include "windows_nodefines.h"
#include <wtypes.h>
#endif /* !PCH_H */

class Dlg_RichPresence
{
public:
    Dlg_RichPresence();
    ~Dlg_RichPresence();

    static INT_PTR CALLBACK s_RichPresenceDialogProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR CALLBACK RichPresenceDialogProc(HWND, UINT, WPARAM, [[maybe_unused]] LPARAM);

    void InstallHWND(HWND hWnd) { m_hRichPresenceDialog = hWnd; }
    HWND GetHWND() const { return m_hRichPresenceDialog; }

    void StartMonitoring();
    void ClearMessage();

    void OnLoad_NewRom();

private:
    void StartTimer();
    void StopTimer();

    HFONT m_hFont;
    HWND m_hRichPresenceDialog;
    bool m_bTimerActive;
};

extern Dlg_RichPresence g_RichPresenceDialog;




#endif // !RA_DLG_RICHPRESENCE_H
