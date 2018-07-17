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
void WM_Handler::OnClose(HWND hwnd) noexcept
{
    ::EndDialog(hwnd, IDCLOSE);
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
    auto result = ::GetWindowLongPtr(hwnd, DWLP_MSGRESULT);
    ::PostQuitMessage(result);
}

_Use_decl_annotations_
void WM_Handler::OnNCDestroy(HWND hwnd) noexcept
{
    if (GetWindowFont(hwnd) != nullptr)
        DeleteFont(GetWindowFont(hwnd));

    ::DestroyCaret();

    if (::GetClassLongPtr(hwnd, GCLP_HCURSOR) == GCLP_HCURSOR)
        ::DestroyCursor(::GetCursor());
    ::DestroyWindow(hwnd);

    // Check if its a window or dialog
    // Check if it's a dialog window first since GWLP_USERDATA has info on the window class
    if (::GetWindowLongPtr(hwnd, DWLP_USER) == DWLP_USER)
        ::SetWindowLongPtr(hwnd, DWLP_USER, 0);
    else if (::GetWindowLongPtr(hwnd, GWLP_USERDATA) == GWLP_USERDATA)
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);


    hwnd = nullptr;
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
    auto hdc = GetDCEx(hwnd, hrgn, DCX_WINDOW | DCX_INTERSECTRGN);
    // Paint into this DC 
    ReleaseDC(hwnd, hdc);
}

_Use_decl_annotations_
BOOL WM_Handler::OnEraseBkgnd(HWND hwnd, [[maybe_unused]] HDC hdc) noexcept
{
    // if it returns 0, the background will be erased if it's GCLP_HBRBACKGROUND it will not be
    return ::GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
}

_Use_decl_annotations_
LPCTSTR CALLBACK GetCaption(HWND hwnd) noexcept
{
    auto len{ ::GetWindowTextLength(hwnd) + 1 };
    auto buffer{ std::make_unique<TCHAR[]>(len) };
    ::GetWindowText(hwnd, buffer.get(), len);
    return buffer.release();
}

} // namespace ui
} // namespace ra
