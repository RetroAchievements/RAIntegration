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

    ~ControlBinding() noexcept
    {
        if (m_hWnd)
            m_pDialog->RemoveControlBinding(m_hWnd);
    }

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
    /// Called when the bound control loses keyboard focus.
    /// </summary>
    GSL_SUPPRESS_F6 virtual void LostFocus() {}
    
    /// <summary>
    /// Called when the bound control's command is activated.
    /// </summary>
    GSL_SUPPRESS_F6 virtual void OnCommand() {}

protected:
    HWND m_hWnd{};

private:
    DialogBase* m_pDialog{};
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_CONTROLBINDING_H
