#include "RA_Dlg_RomChecksum.h"

#include "RA_Resource.h"
#include "RA_Core.h"

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
                    //	Allocate memory to be managed by the clipboard
                    // wrapped just in case it goes out of scope early
                    using GlobalOwner = std::unique_ptr<std::remove_pointer_t<HGLOBAL>, decltype(&::GlobalFree)>;
                    GlobalOwner hMem{ ::GlobalAlloc(GMEM_MOVEABLE, g_sCurrentROMMD5.length() + 1), ::GlobalFree };
                    if (hMem)
                    {
                        const auto& lockResult{ ::GlobalLock(hMem.get()) };
                        if (lockResult)
                            std::memcpy(lockResult, g_sCurrentROMMD5.c_str(), g_sCurrentROMMD5.length() + 1);
                    }
                    else
                        return TRUE;

                    // Might as well check everything
                    if ((::GlobalUnlock(hMem.get()) != 0) || (::GetLastError() != NO_ERROR))
                        return TRUE;
                    if (::OpenClipboard(nullptr) == 0)
                        return TRUE;
                    if (::EmptyClipboard() == 0)
                        return TRUE;
                    if (::SetClipboardData(CF_TEXT, hMem.get()) == nullptr)
                        return TRUE;
                    if (::CloseClipboard() == 0)
                        return TRUE;
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
