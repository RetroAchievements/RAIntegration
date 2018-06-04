#include "RA_Dlg_RomChecksum.h"

#include "RA_Resource.h"
#include "RA_Core.h"

//static 
BOOL RA_Dlg_RomChecksum::DoModalDialog()
{
    return DialogBox(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ROMCHECKSUM), g_RAMainWnd, RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc);
}

INT_PTR CALLBACK RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
            SetDlgItemText(hDlg, IDC_RA_ROMCHECKSUMTEXT, NativeStr(std::string("ROM Checksum: ") + g_sCurrentROMMD5).c_str());
            return FALSE;

        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    EndDialog(hDlg, TRUE);
                    return TRUE;

                case IDC_RA_COPYCHECKSUMCLIPBOARD:
                {
                    // https://msdn.microsoft.com/en-us/library/windows/desktop/aa366574(v=vs.85).aspx
                    // Decided to use HeapCreate instead of HeapAlloc because previously the memory needed to be movable
                    //	Allocate memory to be managed by the clipboard
                    using HeapH = std::unique_ptr<
                        std::remove_pointer_t<HANDLE>,
                        decltype(&::HeapDestroy)
                    >;
                    HeapH hMem{
                        HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0U, g_sCurrentROMMD5.length() + 1U),
                        &::HeapDestroy
                    };

                    {
                        HeapLock(hMem.get());
                        memcpy(hMem.get(), g_sCurrentROMMD5.c_str(), g_sCurrentROMMD5.length() + 1);
                        HeapUnlock(hMem.get());
                    }

                    OpenClipboard(nullptr);
                    EmptyClipboard();
                    SetClipboardData(CF_TEXT, hMem.get());
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
