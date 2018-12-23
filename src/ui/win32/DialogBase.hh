#ifndef RA_UI_WIN32_DIALOGBASE_H
#define RA_UI_WIN32_DIALOGBASE_H
#pragma once

#include "ui\WindowViewModelBase.hh"
#include "ui\win32\IDialogPresenter.hh"
#include "ui\win32\bindings\WindowBinding.hh"

namespace ra {
namespace ui {
namespace win32 {

namespace bindings {
class ControlBinding; // forward declaration
}

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
    _NODISCARD HWND CreateDialogWindow(_In_ const TCHAR* restrict sResourceId,
                                       _In_ IDialogPresenter* const restrict pDialogPresenter);

    /// <summary>
    /// Creates the dialog window and does not return until the window is closed.
    /// </summary>
    /// <param name="sResourceId">The resource identifier defining the dialog.</param>
    /// <param name="pDialogClosed">Callback to call when the dialog is closed.</param>
    void CreateModalWindow(_In_ const TCHAR* restrict sResourceId,
                           _In_ IDialogPresenter* const restrict pDialogPresenter) noexcept;

    /// <summary>
    /// Gets the <see cref="HWND" /> for the dialog.
    /// </summary>
    _NODISCARD HWND GetHWND() const noexcept { return m_hWnd; }

    /// <summary>
    /// Shows the dialog window.
    /// </summary>
    /// <returns><c>true</c> if the window was shown, <c>false</c> if CreateDialogWindow has not been called.</returns>
    bool ShowDialogWindow() const noexcept
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
    _NODISCARD virtual INT_PTR CALLBACK DialogProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM);

    /// <summary>
    /// Called when the window is created, but before it is shown.
    /// </summary>
    /// <returns>Return <c>TRUE</c> if passing the keyboard focus to a default control, otherwise return
    /// <c>FALSE</c>.</returns>
    GSL_SUPPRESS_F6 virtual BOOL OnInitDialog() { return TRUE; }

    /// <summary>
    /// Called when the window is shown.
    /// </summary>
    GSL_SUPPRESS_F6 virtual void OnShown() {}

    /// <summary>
    /// Called when the window is destroyed.
    /// </summary>
    virtual void OnDestroy();

    /// <summary>
    /// Called when a button is clicked.
    /// </summary>
    /// <param name="nCommand">The unique identifier of the button.</param>
    virtual BOOL OnCommand(_In_ WORD nCommand);

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
    friend static INT_PTR CALLBACK StaticDialogProc(_In_ HWND hDlg,
                                                    _In_ UINT uMsg,
                                                    _In_ WPARAM wParam,
                                                    _In_ LPARAM lParam);
    friend static INT_PTR CALLBACK StaticModalDialogProc(_In_ HWND hDlg,
                                                         _In_ UINT uMsg,
                                                         _In_ WPARAM wParam,
                                                         _In_ LPARAM lParam);

    HWND m_hWnd = nullptr;
    IDialogPresenter* m_pDialogPresenter = nullptr; // nullable reference, not allocated
    bool m_bModal = false;

    // allow ControlBinding to access AddControlBinding and RemoveControlBinding methods
    friend class ra::ui::win32::bindings::ControlBinding;
    void AddControlBinding(HWND hControl, ra::ui::win32::bindings::ControlBinding& pControlBinding) noexcept
    {
        GSL_SUPPRESS_F6 m_mControlBindings.insert_or_assign(hControl, &pControlBinding);
    }

    void RemoveControlBinding(HWND hControl) noexcept { GSL_SUPPRESS_F6 m_mControlBindings.erase(hControl); }

    ra::ui::win32::bindings::ControlBinding* FindControlBinding(HWND hControl)
    {
        const auto pIter = m_mControlBindings.find(hControl);
        return (pIter != m_mControlBindings.end() ? pIter->second : nullptr);
    }

    std::unordered_map<HWND, ra::ui::win32::bindings::ControlBinding*> m_mControlBindings;
};

} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_DIALOGBASE_H
