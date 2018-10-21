#ifndef RA_UI_WIN32_DIALOGBASE_H
#define RA_UI_WIN32_DIALOGBASE_H
#pragma once

#include "ui\WindowViewModelBase.hh"
#include "ui\win32\IDialogPresenter.hh"
#include "ui\win32\bindings\WindowBinding.hh"

namespace ra {
namespace ui {
namespace win32 {

class DialogBase
{
public:
    DialogBase() = delete;
    DialogBase(const DialogBase&) noexcept = delete;
    DialogBase& operator=(const DialogBase&) noexcept = delete;
    DialogBase(DialogBase&&) noexcept = delete;
    DialogBase& operator=(DialogBase&&) noexcept = delete;

    /// <summary>
    /// Creates the dialog window (but does not show it).
    /// </summary>
    /// <param name="sResourceId">The resource identifier defining the dialog.</param>
    /// <param name="pDialogClosed">Callback to call when the dialog is closed.</param>
    /// <returns>Handle of the window.</returns>
    /*_NODISCARD */HWND CreateDialogWindow(_In_ const LPCTSTR sResourceId,
                                           _In_ IDialogPresenter* const pDialogPresenter);
    
    /// <summary>
    /// Gets the <see cref="HWND" /> for the dialog.
    /// </summary>
	_NODISCARD HWND GetHWND() const { return m_hWnd; }
    
    /// <summary>
    /// Shows the dialog window.
    /// </summary>
    /// <returns><c>true</c> if the window was shown, <c>false</c> if CreateDialogWindow has not been called.</returns>
	_NODISCARD bool ShowDialogWindow() const
    {
        if (!m_hWnd)
            return false;

        ::ShowWindow(m_hWnd, SW_SHOW);
        return true;
    }

protected:
    explicit DialogBase(_Inout_ ra::ui::WindowViewModelBase& vmWindow) noexcept;
    ~DialogBase() noexcept;

    /// <summary>
    /// Callback for processing <c>WINAPI</c> messages - do not call directly!
    /// </summary>
    _NODISCARD virtual INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);

    /// <summary>
    /// Called when the window is created, but before it is shown.
    /// </summary>
    /// <returns>Return <c>TRUE</c> if passing the keyboard focus to a default control, otherwise return <c>FALSE</c>.</returns>
    virtual BOOL OnInitDialog() { return TRUE; }

    /// <summary>
    /// Called when the window is moved.
    /// </summary>
    virtual void OnDestroy();
    
    /// <summary>
    /// Called when a button is clicked.
    /// </summary>
    /// <param name="nCommand">The unique identifier of the button.</param>
    virtual void OnCommand(_In_ int id);
    
    /// <summary>
    /// Called when the window is moved.
    /// </summary>
    /// <param name="oNewPosition">The new upperleft corner of the client area.</param>
    virtual void OnMove(_In_ const ra::ui::Position& oNewPosition);

    /// <summary>
    /// Called when the window is resized.
    /// </summary>
    /// <param name="oNewPosition">The new size of the client area.</param>
    virtual void OnSize(_In_ const ra::ui::Size& oNewSize);

    ra::ui::win32::bindings::WindowBinding m_bindWindow;

    ra::ui::WindowViewModelBase& m_vmWindow;

private:
    // Allows access to `DialogProc` from static helper
	friend static INT_PTR CALLBACK StaticDialogProc(_In_ HWND hDlg, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    HWND m_hWnd = nullptr;
    IDialogPresenter* m_pDialogPresenter = nullptr; // nullable reference, not allocated
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DIALOGBASE_H
