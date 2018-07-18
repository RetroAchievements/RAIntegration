#include "ra_wm_handler.h"

#include <memory>
#include <CommCtrl.h>
#include <WindowsX.h>

namespace ra {
namespace ui {

_Use_decl_annotations_
BOOL WM_Handler::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam) noexcept
{
    ::SetFocus(hwndFocus);
    // If it returns nonzero, the focus will go back to hwnd
    return ::SetWindowLongPtr(hwnd, DWLP_USER, lParam);
}

_Use_decl_annotations_
void WM_Handler::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) noexcept
{
    return FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, SendMessage);
}

_Use_decl_annotations_
// Used on most window types, use OnCloseModal for modal dialogs
void WM_Handler::OnClose(HWND hwnd) noexcept
{
    ::DestroyWindow(hwnd);
    hwnd = nullptr;
}

_Use_decl_annotations_
BOOL WM_Handler::OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) noexcept
{
    auto result{
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(lpCreateStruct->lpCreateParams))
    };

    if (result == GWLP_USERDATA)
        return TRUE;

    return FALSE;
}

_Use_decl_annotations_
BOOL WM_Handler::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct) noexcept
{
    hwnd = ::CreateWindowEx(lpCreateStruct->dwExStyle, lpCreateStruct->lpszClass, lpCreateStruct->lpszName,
                            lpCreateStruct->style, lpCreateStruct->x, lpCreateStruct->y, lpCreateStruct->cx,
                            lpCreateStruct->cy, lpCreateStruct->hwndParent, lpCreateStruct->hMenu,
                            lpCreateStruct->hInstance, lpCreateStruct->lpCreateParams);

    if (hwnd == nullptr)
        return -1;
    return 0;
}

_Use_decl_annotations_
void WM_Handler::OnDestroy(HWND hwnd) noexcept
{
    // Might as well use it, though it should be 0 always
    ::PostQuitMessage(::GetWindowLongPtr(hwnd, DWLP_MSGRESULT));
}

_Use_decl_annotations_
void WM_Handler::OnNCDestroy(HWND hwnd) noexcept
{
    // Destroy/Delete any Fonts/Brushes w/e (not shared) in the actual class or from a child of WM_Handler
    // since we can't assume what a window actually has

    // This can be used for Modals too but not in WM_CLOSE
    if (hwnd != nullptr)
    {
        ::DestroyWindow(hwnd);
        hwnd = nullptr;
    }
}

_Use_decl_annotations_
LRESULT WM_Handler::OnNotify(HWND hwnd, int idFrom, NMHDR* pnmhdr) noexcept
{
    return FORWARD_WM_NOTIFY(hwnd, idFrom, pnmhdr, SendMessage);
}

_Use_decl_annotations_
void WM_Handler::OnTimer(HWND hwnd, UINT id) noexcept { FORWARD_WM_TIMER(hwnd, id, PostMessage); }

_Use_decl_annotations_
void WM_Handler::OnEnable(HWND hwnd, BOOL fEnable) noexcept
{
    ::EnableWindow(hwnd, fEnable);
}

_Use_decl_annotations_
void WM_Handler::OnCancelMode(HWND hwnd) noexcept
{
    ::EnableWindow(hwnd, FALSE);
}

_Use_decl_annotations_
HFONT WM_Handler::OnGetFont(HWND hwnd) noexcept { return GetWindowFont(hwnd); }

_Use_decl_annotations_
void WM_Handler::OnSetFont(HWND hwndCtl, HFONT hfont, BOOL fRedraw) noexcept
{
    SetWindowFont(hwndCtl, hfont, fRedraw);
}

_Use_decl_annotations_
INT WM_Handler::OnGetText(HWND hwnd, int cchTextMax, LPTSTR lpszText) noexcept
{
    return ::GetWindowText(hwnd, lpszText, cchTextMax);
}

_Use_decl_annotations_
void WM_Handler::OnSetText(HWND hwnd, LPCTSTR lpszText) noexcept { ::SetWindowText(hwnd, lpszText); }

_Use_decl_annotations_
INT WM_Handler::OnGetTextLength(HWND hwnd) noexcept { return ::GetWindowTextLength(hwnd); }

_Use_decl_annotations_
void WM_Handler::OnSetFocus(HWND hwnd, HWND hwndOldFocus) noexcept
{
    // Just for the sake of using it (compiler warning)
    ::SetFocus(hwndOldFocus);
    ::SetFocus(hwnd);
}

_Use_decl_annotations_
void WM_Handler::OnKillFocus(HWND hwnd, HWND hwndNewFocus) noexcept
{
    this->OnSetFocus(hwndNewFocus, hwnd);
}

_Use_decl_annotations_
void WM_Handler::OnSetRedraw(HWND hwnd, BOOL fRedraw) noexcept
{
    SetWindowRedraw(hwnd, fRedraw);
}

_Use_decl_annotations_
void WM_Handler::OnShowWindow(HWND hwnd, BOOL fShow, UINT status) noexcept
{
    FORWARD_WM_SHOWWINDOW(hwnd, fShow, status, PostMessage);
}

_Use_decl_annotations_
UINT WM_Handler::OnGetDlgCode(HWND hwnd, LPMSG lpmsg) noexcept
{
    return FORWARD_WM_GETDLGCODE(hwnd, lpmsg, SendMessage);
}

_Use_decl_annotations_
HWND WM_Handler::OnNextDlgCtl(HWND hwnd, HWND hwndSetFocus, BOOL fNext) noexcept
{
    // strangely the docs said to post it
    return FORWARD_WM_NEXTDLGCTL(hwnd, hwndSetFocus, fNext, PostMessage);
}

_Use_decl_annotations_
void WM_Handler::OnEnterIdle(HWND hwnd, UINT source, HWND hwndSource) noexcept
{
    FORWARD_WM_ENTERIDLE(hwnd, source, hwndSource, ::DefWindowProc);
}

_Use_decl_annotations_
void WM_Handler::OnNCPaint(HWND hwnd, HRGN hrgn) noexcept
{
    using WindowDC = std::unique_ptr<HDC__ , decltype(&::DeleteDC)>;
    WindowDC hdc{ GetDCEx(hwnd, hrgn, DCX_WINDOW | DCX_INTERSECTRGN), ::DeleteDC };
    ReleaseDC(hwnd, hdc.release());
}

_Use_decl_annotations_
BOOL WM_Handler::OnEraseBkgnd(HWND hwnd, [[maybe_unused]] HDC hdc) noexcept
{
    // if it returns 0, the background will be erased if it's GCLP_HBRBACKGROUND it will not be
    return ::GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
}


#pragma region Common Button Handlers
void WM_Handler::OnCloseModal(HWND hDlg) noexcept { ::EndDialog(hDlg, IDCLOSE); }

_Use_decl_annotations_
void WM_Handler::OnOK(HWND hDlg) noexcept { ::EndDialog(hDlg, IDOK); }

_Use_decl_annotations_
void WM_Handler::OnCancel(HWND hDlg) noexcept { ::EndDialog(hDlg, IDCANCEL); }

_Use_decl_annotations_
void WM_Handler::OnAbort(HWND hDlg) noexcept { ::EndDialog(hDlg, IDABORT); }

_Use_decl_annotations_
void WM_Handler::OnRetry(HWND hDlg) noexcept { ::EndDialog(hDlg, IDRETRY); }

_Use_decl_annotations_
void WM_Handler::ButtonCheck(HWND hwnd) noexcept { Button_SetCheck(hwnd, BST_CHECKED); }

_Use_decl_annotations_
void WM_Handler::ButtonUncheck(HWND hwnd) noexcept { Button_SetCheck(hwnd, BST_UNCHECKED); }

_Use_decl_annotations_
void WM_Handler::DisableButtonCheck(HWND hwnd) noexcept { Button_SetCheck(hwnd, BST_INDETERMINATE); }
#pragma endregion


_Use_decl_annotations_
LPCTSTR CALLBACK GetCaption(HWND hwnd) noexcept
{
    auto len{ ::GetWindowTextLength(hwnd) + 1 };
    auto buffer{ std::make_unique<TCHAR[]>(len) };
    ::GetWindowText(hwnd, buffer.get(), len);
    return buffer.release();
}

_Use_decl_annotations_
LPCTSTR CALLBACK GetActiveWindowCaption() noexcept
{
    return GetCaption(::GetActiveWindow());
}




} // namespace ui
} // namespace ra
