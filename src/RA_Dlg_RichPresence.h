#ifndef RA_DLG_RICHPRESENCE_H
#define RA_DLG_RICHPRESENCE_H
#pragma once

#pragma pack(push, 1)
class Dlg_RichPresence
{
public:
    Dlg_RichPresence();
    ~Dlg_RichPresence();

    static INT_PTR CALLBACK s_RichPresenceDialogProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR CALLBACK RichPresenceDialogProc(HWND, UINT, WPARAM, LPARAM);

    void InstallHWND(HWND hWnd) { m_hRichPresenceDialog = hWnd; }
    HWND GetHWND() const { return m_hRichPresenceDialog; }

    void StartMonitoring();

private:
    void StartTimer();
    void StopTimer();

    HFONT m_hFont{ nullptr };
    HWND m_hRichPresenceDialog{ nullptr };
    bool m_bTimerActive{ false };
};
#pragma pack(pop)

extern Dlg_RichPresence g_RichPresenceDialog;




#endif // !RA_DLG_RICHPRESENCE_H
