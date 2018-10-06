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
    /// Gets the <see cref="HWND" /> for the dialog.
    /// </summary>
    HWND GetHWND() const { return m_hWnd; }
    
    /// <summary>
    /// Callback for procesing WINAPI messages - do not call directly!
    /// </summary>
    virtual INT_PTR CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);
    
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

    DialogBase(ra::ui::WindowViewModelBase& vmWindow) noexcept;
    virtual ~DialogBase();

    virtual void OnInitDialog() {}
    virtual void OnDestroy();
    virtual BOOL OnCommand(WORD nCommand);
    virtual void OnMove(ra::ui::Position& oNewPosition);
    virtual void OnSize(ra::ui::Size& oNewSize);

    ra::ui::win32::bindings::WindowBinding m_bindWindow;

    ra::ui::WindowViewModelBase& m_vmWindow;

private:
    HWND m_hWnd;
    IDialogPresenter* m_pDialogPresenter = nullptr; // nullable reference, not allocated
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DIALOGBASE_H
