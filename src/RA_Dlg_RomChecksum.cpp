#include "RA_Dlg_RomChecksum.h"

#include "RA_Resource.h"
#include "RA_Core.h"

#include "data\GameContext.hh"

//static 
INT_PTR RA_Dlg_RomChecksum::DoModalDialog() noexcept
{
    return DialogBox(g_hThisDLLInst, MAKEINTRESOURCE(IDD_RA_ROMCHECKSUM), g_RAMainWnd, RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc);
}

INT_PTR CALLBACK RA_Dlg_RomChecksum::RA_Dlg_RomChecksumProc(HWND hDlg, UINT nMsg, WPARAM wParam, [[maybe_unused]] LPARAM)
{
    switch (nMsg)
    {
        case WM_INITDIALOG:
        {
            const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
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
                    
                    const auto& pGameContext = ra::services::ServiceLocator::Get<ra::data::GameContext>();
                    using Global = std::unique_ptr<std::remove_pointer_t<HGLOBAL>, decltype(&::GlobalFree)>;

                    //	Allocate memory to be managed by the clipboard
                    Global hMem{ ::GlobalAlloc(GMEM_MOVEABLE, pGameContext.GameHash().length() + 1), ::GlobalFree };

                    if (hMem)
                    {
                        // It's not auto-deducing what we need, the pointer should not be modified at all
                        void* const lockResult{ ::GlobalLock(hMem.get()) };
                        if (lockResult)
                            std::memcpy(lockResult, pGameContext.GameHash().c_str(), pGameContext.GameHash().length() + 1);
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
                    // According to docs, the system owns the handle if it failed
                    if (::SetClipboardData(CF_TEXT, hMem.release()) == nullptr)
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
