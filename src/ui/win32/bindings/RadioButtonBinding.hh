#ifndef RA_UI_WIN32_RADIOBUTTONBINDING_H
#define RA_UI_WIN32_RADIOBUTTONBINDING_H
#pragma once

#include "ControlBinding.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class RadioButtonBinding : public ControlBinding
{
public:
    explicit RadioButtonBinding(ViewModelBase& vmViewModel) noexcept : ControlBinding(vmViewModel) {}

    void SetHWND(DialogBase& pDialog, HWND hControl) override
    {
        ControlBinding::SetHWND(pDialog, hControl);

        if (m_pIsCheckedProperty)
            Button_SetCheck(hControl, GetValue(*m_pIsCheckedProperty) == m_nIsCheckedValue ? BST_CHECKED : BST_UNCHECKED);
    }

    void BindCheck(const IntModelProperty& pSourceProperty, int nIsCheckedValue)
    {
        m_pIsCheckedProperty = &pSourceProperty;
        m_nIsCheckedValue = nIsCheckedValue;

        if (m_hWnd)
            Button_SetCheck(m_hWnd, GetValue(*m_pIsCheckedProperty) == m_nIsCheckedValue ? BST_CHECKED : BST_UNCHECKED);
    }

    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override 
    {
        if (args.Property == *m_pIsCheckedProperty)
            Button_SetCheck(m_hWnd, GetValue(*m_pIsCheckedProperty) == m_nIsCheckedValue ? BST_CHECKED : BST_UNCHECKED);
    }

    void OnCommand() override
    {
        SetValue(*m_pIsCheckedProperty, m_nIsCheckedValue);
    }

private:
    const IntModelProperty* m_pIsCheckedProperty = nullptr;
    int m_nIsCheckedValue = 0;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_RADIOBUTTONBINDING_H
