#include "RA_Dlg_RomChecksum.h"

#include "RA_Resource.h"
#include "RA_Core.h"

//static 
INT_PTR RA_Dlg_RomChecksum::DoModalDialog() noexcept
{
    return DialogBox(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ROMCHECKSUM), g_RAMainWnd, RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc);
}

INT_PTR CALLBACK RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc(HWND hDlg, UINT nMsg, WPARAM wParam, [[maybe_unused]] LPARAM)
{
    const auto EndDialogAsync =[hDlg](int id) noexcept
    {
        if (const auto nResult{ ::EndDialog(hDlg, id) }; nResult != 0)
            return nResult; // succeeded
        return 0; // failed
    };

    using Global = std::unique_ptr<void, decltype(&::GlobalFree)>;
    const auto SetClipboard =[](Global& hMem) noexcept
    {
        auto hMemRaw{ hMem.release() };
        if (::SetClipboardData(ra::flip_sign(CF_TEXT), hMemRaw) == nullptr)
        {
            if(hMemRaw)
            {
                ::GlobalFree(hMemRaw);
                hMemRaw = nullptr;
            }
            return false; // failed
        }
        // Move it back into hMem if the operation succeeded so it can delete it for us
        hMem ={ hMemRaw, ::GlobalFree };
        return true; // succeeded
    };

    switch (nMsg)
    {
        case WM_INITDIALOG:
        {
            std::string str{ "ROM Checksum: " };
            str.append(g_sCurrentROMMD5);
            SetDlgItemText(hDlg, IDC_RA_ROMCHECKSUMTEXT, str.c_str());
            return FALSE;
        }

        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDOK:
                    return EndDialogAsync(IDOK);

                case IDC_RA_COPYCHECKSUMCLIPBOARD:
                {
                    // Allocate memory to be managed by the clipboard
                    // wrapped just in case it goes out of scope early                    
                    Global hMem{ ::GlobalAlloc(GMEM_MOVEABLE, g_sCurrentROMMD5.length() + 1), ::GlobalFree };
                    if (hMem)
                    {
                        auto lockResult{ ::GlobalLock(hMem.get()) };
                        if (lockResult)
                        {
                            std::memcpy(lockResult, g_sCurrentROMMD5.c_str(), g_sCurrentROMMD5.length() + 1);
                            lockResult = nullptr;
                        }
                    }
                    else
                        return -1;

                    // Might as well check everything
                    if ((::GlobalUnlock(hMem.get()) != FALSE) || (::GetLastError() != NO_ERROR))
                        return -1;
                    if (::OpenClipboard(nullptr) == FALSE)
                        return -1;
                    if (::EmptyClipboard() == FALSE)
                        return -1;
                    if (SetClipboard(hMem) != true)
                        return -1;
                    if (::CloseClipboard() == FALSE)
                        return -1;
                }
            }
        }break;

        case WM_CLOSE:
            return EndDialogAsync(IDCLOSE);
    }
    return 0;
}
