#ifndef RA_WM_HANDLER_H
#define RA_WM_HANDLER_H
#pragma once

#include <WTypes.h>
#include <yvals.h>

#ifndef _NORETURN
#define _NORETURN
#endif // !_NORETURN

// References:
// https://docs.microsoft.com/en-us/windows/desktop/api/Winuser/nf-winuser-defdlgprocw

namespace ra {
namespace ui {

// We can make this an abstract interface later
/// <summary>This class serves as a base Windows Message handler.</summary>
/// <remarks>
///   Only Messages that are guaranteed to be processed or have the same behavior
///   will be here. Any additional handlers should derive this class
/// </remarks>
struct WM_Handler
{
#pragma region Constructors
    WM_Handler() noexcept = default;
    virtual ~WM_Handler() noexcept = default;
    WM_Handler(const WM_Handler&) = delete;
    WM_Handler& operator=(const WM_Handler&) = delete;
    inline constexpr WM_Handler(WM_Handler&&) noexcept = default;
    inline constexpr WM_Handler& operator=(WM_Handler&&) noexcept = default;
#pragma endregion

    // Basic rule: Messages with pointers (the first hwnd is not part of it) are
    //             sent, otherwise they are posted. If you don't want it to processed immediate post it the message queue, but make sure it has no pointers.
#pragma region Event Handlers
    // These should be overridden

    // This base method should only be used in the dialogs message queue and not it's procedure 
    _NODISCARD virtual BOOL OnInitDialog(_In_ HWND hwnd, _In_ HWND hwndFocus, _In_ LPARAM lParam) noexcept;
    _NORETURN virtual void OnCommand(_In_ HWND hwnd, _In_ int id, _In_ HWND hwndCtl, _In_ UINT codeNotify) noexcept;
    _NORETURN virtual void OnClose(_In_ HWND hwnd) noexcept;
    _NODISCARD virtual BOOL OnNCCreate(_In_ HWND hwnd, _In_ LPCREATESTRUCT lpCreateStruct) noexcept;
    _NODISCARD virtual BOOL OnCreate(_In_ HWND hwnd, _In_ LPCREATESTRUCT lpCreateStruct) noexcept;
    _NORETURN virtual void OnDestroy(_In_ HWND hwnd) noexcept;
    _NORETURN virtual void OnNCDestroy(_In_ HWND hwnd) noexcept;
    _NODISCARD virtual LRESULT OnNotify(_In_ HWND hwnd, _In_ int idFrom, _In_ NMHDR* pnmdr) noexcept;
    _NORETURN virtual void OnTimer(_In_ HWND hwnd, _In_ UINT id) noexcept;

    // Stuff that probably shouldn't be overridden
    // You could use ::EnableWindow Directly but the default window proc might pass invalid input
    _NORETURN void OnEnable(_In_ HWND hwnd, _In_ BOOL fEnable = TRUE) noexcept;
    _NORETURN void OnCancelMode(_In_ HWND hwnd) noexcept;

    _NODISCARD HFONT OnGetFont(_In_ HWND hwnd) noexcept;
    _NORETURN void OnSetFont(_In_ HWND hwndCtl, _In_ HFONT hfont, _In_ BOOL fRedraw) noexcept;

    _NODISCARD INT OnGetText(_In_ HWND hwnd, _In_ int cchTextMax, _In_ LPTSTR lpszText) noexcept;
    _NORETURN void OnSetText(_In_ HWND hwnd, _In_ LPCTSTR lpszText) noexcept;
    _NODISCARD INT OnGetTextLength(_In_ HWND hwnd) noexcept;

    _NORETURN void OnSetFocus(_In_ HWND hwnd, _In_ HWND hwndOldFocus) noexcept;
    _NORETURN void OnKillFocus(_In_ HWND hwnd, _In_ HWND hwndNewFocus) noexcept;

    _NORETURN void OnSetRedraw(_In_ HWND hwnd, _In_ BOOL fRedraw) noexcept;
    _NORETURN void OnShowWindow(_In_ HWND hwnd, _In_ BOOL fShow, _In_ UINT status) noexcept;
#pragma endregion

    // Do you want to process dialog messages too?
    _NODISCARD UINT OnGetDlgCode(_In_ HWND hwnd, _In_ LPMSG lpmsg) noexcept;
    _NODISCARD HWND OnNextDlgCtl(_In_ HWND hwnd, _In_ HWND hwndSetFocus, _In_ BOOL fNext) noexcept;
    _NORETURN void OnEnterIdle(_In_ HWND hwnd, _In_ UINT source, _In_ HWND hwndSource) noexcept;

    _NORETURN void OnNCPaint(_In_ HWND hwnd, _In_ HRGN hrgn) noexcept;
    _NODISCARD BOOL OnEraseBkgnd(_In_ HWND hwnd, _In_ [[maybe_unused]] HDC hdc) noexcept;
};

// Mostly for debugging but could be used
_NODISCARD LPCTSTR CALLBACK GetCaption(_In_ HWND hwnd) noexcept;

} // namespace ui
} // namespace ra


#endif // !RA_WM_HANDLER_H

