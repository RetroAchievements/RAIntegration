#include "RA_Dlg_RichPresence.h"

#include "RA_Core.h"
#include "RA_Resource.h"
#include "RA_RichPresence.h"

Dlg_RichPresence g_RichPresenceDialog;

INT_PTR CALLBACK Dlg_RichPresence::RichPresenceDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
        {
            m_hFont = FontH{ CreateFont(15, 0, 0, 0, FW_NORMAL, 0UL, 0UL, 0UL,
                ra::to_unsigned(DEFAULT_CHARSET),
                ra::to_unsigned(OUT_DEFAULT_PRECIS), ra::to_unsigned(CLIP_DEFAULT_PRECIS),
                ra::to_unsigned(CLEARTYPE_QUALITY), VARIABLE_PITCH, nullptr),
                reinterpret_cast<FontDeleterType>(&::DeleteObject) };

            m_ctlFont = WindowH{
                ::GetDlgItem(hDlg, IDC_RA_RICHPRESENCERESULTTEXT), &::DestroyWindow
            };
            // did I put release here?
            SetWindowFont(m_ctlFont.get(), m_hFont.get(), TRUE);
            RestoreWindowPosition(hDlg, "Rich Presence Monitor", true, true);
            return TRUE;
        }

        case WM_TIMER:
        {
            std::wstring sRP = Widen(g_RichPresenceInterpretter.GetRichPresenceString());
            // https://msdn.microsoft.com/en-us/library/windows/desktop/ms632644(v=vs.85).aspx (WM_SETTEXT)
            // https://msdn.microsoft.com/en-us/library/windows/desktop/ms644902(v=vs.85).aspx (WM_TIMER)
            if (SetWindowTextW(m_ctlFont.get(), sRP.c_str()) == TRUE)
                return 0;

            return ra::to_signed(::GetLastError());
        }

        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms647591(v=vs.85).aspx (WM_COMMAND)
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                // The close button is tied to IDOK plus this is a modeless dialog
                case IDCLOSE:
                case IDCANCEL:
                    _FALLTHROUGH;
                case IDOK:
                    StopTimer();
                    // We don't have an instance pointer here via
                    // GetWindowLongPointer so the HWND won't get refreshed,
                    // we'll worry about that later. The Window should be
                    // destroyed and recreated according to the API but it won't
                    // work right now.
                    ::EndDialog(hDlg, IDOK);
                    return 0;

                default:
                    return ra::to_signed(::GetLastError());
            }
        }

        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms632620(v=vs.85).aspx (WM_DESTROY)
        case WM_DESTROY:
        {
            StopTimer();
            // Don't destroy it or it won't show up again, it'll get destroyed when you close the app
            // PostQuitMessage pretty much means the current thread wants to quit eventually
            ::PostQuitMessage(0);
        }
        return 0;

        case WM_MOVE:
            RememberWindowPosition(hDlg, "Rich Presence Monitor");
            break;

        case WM_SIZE:
            RememberWindowSize(hDlg, "Rich Presence Monitor");
            break;
    }

    return FALSE;
}

//static 
INT_PTR CALLBACK Dlg_RichPresence::s_RichPresenceDialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return g_RichPresenceDialog.RichPresenceDialogProc(hDlg, uMsg, wParam, lParam);
}


void Dlg_RichPresence::StartMonitoring()
{
    if (g_RichPresenceInterpretter.Enabled())
    {
        StartTimer();
    }
    else
    {
        StopTimer();
    }
}

void Dlg_RichPresence::StartTimer()
{
    if (!m_bTimerActive)
    {
        SetTimer(m_hRichPresenceDialog.get(), 1U, 1000U, nullptr);
        m_bTimerActive = true;
    }
}

void Dlg_RichPresence::StopTimer()
{
    if (m_bTimerActive)
    {
        KillTimer(m_hRichPresenceDialog.get(), 1U);
        m_bTimerActive = false;
    }
}
