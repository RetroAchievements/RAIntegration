#ifndef RA_UI_WIN32_CONTROLBINDING_H
#define RA_UI_WIN32_CONTROLBINDING_H
#pragma once

#include "ui\BindingBase.hh"
#include "ui\win32\DialogBase.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class ControlBinding : protected BindingBase
{
public:
    explicit ControlBinding(ViewModelBase& vmViewModel) noexcept : BindingBase(vmViewModel) {}

    ~ControlBinding() noexcept;

    ControlBinding(const ControlBinding&) noexcept = delete;
    ControlBinding& operator=(const ControlBinding&) noexcept = delete;
    ControlBinding(ControlBinding&&) noexcept = delete;
    ControlBinding& operator=(ControlBinding&&) noexcept = delete;

    /// <summary>
    /// Associates the control for binding.
    /// </summary>
    /// <param name="pDialog">The dialog containing the control.</param>
    /// <param name="hWnd">The unique identifier of the control.</param>
    void SetControl(DialogBase& pDialog, int nControlResourceId)
    {
        SetHWND(pDialog, GetDlgItem(pDialog.GetHWND(), nControlResourceId));
    }
    
    /// <summary>
    /// Associates the control for binding.
    /// </summary>
    /// <param name="pDialog">The dialog containing the control.</param>
    /// <param name="hControl">The control handle.</param>
    GSL_SUPPRESS_F6 virtual void SetHWND(DialogBase& pDialog, HWND hControl)
    {
        m_hWnd = hControl;
        m_pDialog = &pDialog;
        m_pDialog->AddControlBinding(hControl, *this);
    }

    /// <summary>
    /// Called when the control is made visible.
    /// </summary>
    GSL_SUPPRESS_F6 virtual void OnShown() {}

    /// <summary>
    /// Called when the bound control gains keyboard focus.
    /// </summary>
    GSL_SUPPRESS_F6 virtual void OnGotFocus() {}

    /// <summary>
    /// Called when the bound control loses keyboard focus.
    /// </summary>
    GSL_SUPPRESS_F6 virtual void OnLostFocus() {}
    
    /// <summary>
    /// Called when the bound control's command is activated.
    /// </summary>
    GSL_SUPPRESS_F6 virtual void OnCommand() {}

    /// <summary>
    /// Called when the bound control's value changed.
    /// </summary>
    GSL_SUPPRESS_F6 virtual void OnValueChanged() {}

    /// <summary>
    /// Called when the bound control is resized.
    /// </summary>
    GSL_SUPPRESS_F6 virtual void OnSizeChanged(_UNUSED const ra::ui::Size& pNewSize) {}

    /// <summary>
    /// Called when the bound control offers custom drawing.
    /// </summary>
    GSL_SUPPRESS_F6 virtual LRESULT OnCustomDraw(_UNUSED NMLVCUSTOMDRAW* pCustomDraw)
    {
        return CDRF_DODEFAULT;
    }

    /// <summary>
    /// If set to true, UpdateWindow will be called when a control is Invalidated.
    /// </summary>
    static void SetNeedsUpdateWindow(bool bValue) noexcept { s_bNeedsUpdateWindow = bValue; }

    /// <summary>
    /// Determines if UpdateWindow should be called when invalidating a control.
    /// </summary>
    static bool NeedsUpdateWindow() noexcept { return s_bNeedsUpdateWindow; }

    /// <summary>
    /// Calls Invalidate() or UpdateWindow() to cause the control to repaint
    /// </summary>
    static void ForceRepaint(HWND hWnd);

    /// <summary>
    /// Calls UpdateWindow() to cause the control to repaint
    /// </summary>
    static void RedrawWindow(HWND hWnd);

    /// <summary>
    /// Delays ForceRepaint calls until resumed.
    /// </summary>
    static void SuspendRepaint();

    /// <summary>
    /// Resumes ForceRepaint calls.
    /// </summary>
    static void ResumeRepaint();

    class RepaintGuard final
    {
    public:
        RepaintGuard() noexcept
        {
            GSL_SUPPRESS_F6 ControlBinding::SuspendRepaint();
        }

        ~RepaintGuard() noexcept
        {
            GSL_SUPPRESS_F6 ControlBinding::ResumeRepaint();
        }

        RepaintGuard(const RepaintGuard&) noexcept = delete;
        RepaintGuard& operator=(const RepaintGuard&) noexcept = delete;
        RepaintGuard(RepaintGuard&&) noexcept = delete;
        RepaintGuard& operator=(RepaintGuard&&) noexcept = delete;
    };

    /// <summary>
    /// DO NOT CALL! Must be public for WINAPI interop.
    /// </summary>
    _NODISCARD virtual INT_PTR CALLBACK WndProc(_In_ HWND, _In_ UINT, _In_ WPARAM, _In_ LPARAM) noexcept(false);

    static void DetachSubclasses() noexcept;

protected:
    void DisableBinding() noexcept
    {
        if (m_hWnd && m_pDialog)
            m_pDialog->RemoveControlBinding(m_hWnd);
    }

    void EnableBinding() noexcept
    {
        if (m_hWnd && m_pDialog)
            m_pDialog->AddControlBinding(m_hWnd, *this);
    }

    void InvokeOnUIThread(std::function<void()> fAction)
    {
        WindowBinding::InvokeOnUIThread(fAction);
    }

    void InvokeOnUIThreadAndWait(std::function<void()> fAction)
    {
        WindowBinding::InvokeOnUIThreadAndWait(fAction);
    }

    HWND GetDialogHwnd() const
    {
        Expects(m_pDialog != nullptr);
        return m_pDialog->m_hWnd;
    }

    void SubclassWndProc();
    void UnsubclassWndProc() noexcept;

    void AddSecondaryControlBinding(HWND hWnd) noexcept
    {
        if (hWnd && m_pDialog)
            m_pDialog->AddControlBinding(hWnd, *this);
    }

    void RemoveSecondaryControlBinding(HWND hWnd) noexcept
    {
        if (hWnd && m_pDialog)
            m_pDialog->RemoveControlBinding(hWnd);
    }

    HWND m_hWnd{};

private:
    DialogBase* m_pDialog = nullptr;
    WNDPROC m_pOriginalWndProc = nullptr;

    static bool s_bNeedsUpdateWindow;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_CONTROLBINDING_H
