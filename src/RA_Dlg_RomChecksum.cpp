#include "RA_Dlg_RomChecksum.h"

#include "RA_Resource.h"
#include "RA_Core.h"

#include "services\IGameContext.hh"

//static 
BOOL RA_Dlg_RomChecksum::DoModalDialog()
{
    return DialogBox(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ROMCHECKSUM), g_RAMainWnd, RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc);
}

INT_PTR CALLBACK RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc(HWND hDlg, UINT nMsg, WPARAM wParam, [[maybe_unused]] LPARAM)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
        {
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::services::IGameContext>();
            SetDlgItemText(hDlg, IDC_RA_ROMCHECKSUMTEXT, NativeStr("ROM Checksum: " + pGameContext.GameHash()).c_str());
            return FALSE;
        }

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDC_RA_COPYCHECKSUMCLIPBOARD:
                {
                    //	Allocate memory to be managed by the clipboard
                    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::services::IGameContext>();
                    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, pGameContext.GameHash().length() + 1);
                    memcpy(GlobalLock(hMem), pGameContext.GameHash().c_str(), pGameContext.GameHash().length() + 1);
                    GlobalUnlock(hMem);

                    OpenClipboard(nullptr);
                    EmptyClipboard();
                    SetClipboardData(CF_TEXT, hMem);
                    CloseClipboard();

                    //MessageBeep( 0xffffffff );
                }
                return TRUE;

                default:
                    return FALSE;
            }

        case WM_CLOSE:
            EndDialog(hDlg, TRUE);
            return TRUE;

        default:
            return FALSE;
    }
}
