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
    /// <summary>
    /// Creates the dialog window (but does not show it).
    /// </summary>
    /// <param name="sResourceId">The resource identifier defining the dialog.</param>
    /// <param name="pDialogClosed">Callback to call when the dialog is closed.</param>
    /// <returns>Handle of the window.</returns>
    HWND CreateDialogWindow(LPTSTR sResourceId, IDialogPresenter* pDialogPresenter);

    /// <summary>
    /// Creates the dialog window and does not return until the window is closed.
    /// </summary>
    /// <param name="sResourceId">The resource identifier defining the dialog.</param>
    /// <param name="pDialogClosed">Callback to call when the dialog is closed.</param>
    void CreateModalWindow(LPTSTR sResourceId, IDialogPresenter* pDialogPresenter);

    /// <summary>
    /// Gets the <see cref="HWND" /> for the dialog.
    /// </summary>
    HWND GetHWND() const { return m_hWnd; }
    
    /// <summary>
    /// Callback for procesing WINAPI messages - do not call directly!
    /// </summary>
    virtual INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    /// <summary>
    /// Shows the dialog window.
    /// </summary>
    /// <returns><c>true</c> if the window was shown, <c>false</c> if CreateDialogWindow has not been called.</returns>
    bool ShowDialogWindow() const
    {
        if (!m_hWnd)
            return false;

        ShowWindow(m_hWnd, SW_SHOW);
        return true;
    }

protected:
    DialogBase() = delete;

    explicit DialogBase(ra::ui::WindowViewModelBase& vmWindow) noexcept;
    virtual ~DialogBase() noexcept;

    /// <summary>
    /// Called when the window is created, but before it is shown.
    /// </summary>
    virtual void OnInitDialog() {}

    /// <summary>
    /// Called when the window is moved.
    /// </summary>
    virtual void OnDestroy();
    
    /// <summary>
    /// Called when a button is clicked.
    /// </summary>
    /// <param name="nCommand">The unique identifier of the button.</param>
    /// <returns><c>TRUE</c> if the command was handled, <c>FALSE</c> if not.</returns>
    virtual BOOL OnCommand(WORD nCommand);
    
    /// <summary>
    /// Called when the window is moved.
    /// </summary>
    /// <param name="oNewPosition">The new upperleft corner of the client area.</param>
    virtual void OnMove(ra::ui::Position& oNewPosition);

    /// <summary>
    /// Called when the window is resized.
    /// </summary>
    /// <param name="oNewPosition">The new size of the client area.</param>
    virtual void OnSize(ra::ui::Size& oNewSize);

    ra::ui::win32::bindings::WindowBinding m_bindWindow;

    ra::ui::WindowViewModelBase& m_vmWindow;

private:
    HWND m_hWnd = nullptr;
    IDialogPresenter* m_pDialogPresenter = nullptr; // nullable reference, not allocated
    bool m_bModal = false;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DIALOGBASE_H
