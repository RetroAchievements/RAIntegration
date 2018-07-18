#include "RA_Dlg_RomChecksum.h"

#include "RA_Resource.h"
#include "RA_Core.h"
#include "ra_wm_handler.h"

namespace ra {
namespace ui {

INT_PTR Dlg_RomChecksum::DoModal()
{
    // ::GetAncestor(::GetActiveWindow(), GA_ROOT) is needed since
    // GetActiveWindow might return the menu item instead of g_RAMainWnd, it
    // means that it get's the root window.
    return DialogBoxParam(::GetModuleHandle(_T("RA_Integration.dll")), MAKEINTRESOURCE(IDD_RA_ROMCHECKSUM),
                          ::GetAncestor(::GetActiveWindow(), GA_ROOT), Dlg_RomChecksumProc,
                          reinterpret_cast<LPARAM>(this));
}

struct ChecksumHelper : public WM_Handler
{
    BOOL OnInitDialog(HWND hwnd, _UNUSED HWND hwndFocus, _UNUSED LPARAM lParam) noexcept override
    {
        // It seems hwndFocus is the thing with the default focus, (usually OK button)
        using WindowH = std::unique_ptr<HWND__, decltype(&::DestroyWindow)>;
        WindowH hRomChecksum{ ::GetDlgItem(hwnd, IDC_RA_ROMCHECKSUMTEXT), ::DestroyWindow };

        // just pray it doesn't leak, we can't destroy it or we won't see anything
        OnSetText(hRomChecksum.release(), NativeStr("ROM Checksum: " + g_sCurrentROMMD5).c_str());
        return FALSE;
    }

    void OnCommand(HWND hwnd, int id, _UNUSED HWND hwndCtl, _UNUSED UINT codeNotify) noexcept override
    {
        switch (id)
        {
            case IDOK:
                OnOK(hwnd);
                break;

            case IDC_RA_COPYCHECKSUMCLIPBOARD:
            {
                //	Allocate memory to be managed by the clipboard
                using GlobalH = std::unique_ptr<void, decltype(&::GlobalFree)>;

                // Since we're using CF_TEXT we have to free the memory using GlobalFree (which wasn't done before)
                GlobalH hMem{ ::GlobalAlloc(GMEM_MOVEABLE, g_sCurrentROMMD5.length() + 1), ::GlobalFree };

                // Do we really need memcpy?
                ::memcpy(::GlobalLock(hMem.get()),
                         static_cast<const void*>(g_sCurrentROMMD5.c_str()),
                         g_sCurrentROMMD5.length() + 1U);
                ::GlobalUnlock(hMem.get());

                ::OpenClipboard(nullptr);
                ::EmptyClipboard();
                ::SetClipboardData(CF_TEXT, hMem.get());
                ::CloseClipboard();
            }
        }
    }
};

INT_PTR CALLBACK Dlg_RomChecksumProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (auto helper = std::make_unique<ChecksumHelper>(); nMsg)
    {
        HANDLE_MSG(hDlg, WM_INITDIALOG, helper->OnInitDialog);
        HANDLE_MSG(hDlg, WM_COMMAND, helper->OnCommand);
        HANDLE_MSG(hDlg, WM_CLOSE, helper->OnCloseModal);
        default:
            return FALSE;
    }
}

} // namespace ui
} // namespace ra
