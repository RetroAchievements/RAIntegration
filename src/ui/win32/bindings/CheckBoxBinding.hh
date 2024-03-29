#ifndef RA_UI_WIN32_CHECKBOXBINDING_H
#define RA_UI_WIN32_CHECKBOXBINDING_H
#pragma once

#include "ControlBinding.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class CheckBoxBinding : public ControlBinding
{
public:
    explicit CheckBoxBinding(ViewModelBase& vmViewModel) noexcept : ControlBinding(vmViewModel) {}

    void SetHWND(DialogBase& pDialog, HWND hControl) override
    {
        ControlBinding::SetHWND(pDialog, hControl);

        if (m_pIsCheckedProperty)
            Button_SetCheck(hControl, GetValue(*m_pIsCheckedProperty) ? BST_CHECKED : BST_UNCHECKED);
    }

    void BindCheck(const BoolModelProperty& pSourceProperty)
    {
        m_pIsCheckedProperty = &pSourceProperty;

        if (m_hWnd)
            Button_SetCheck(m_hWnd, GetValue(*m_pIsCheckedProperty) ? BST_CHECKED : BST_UNCHECKED);
    }

    void OnCommand() override
    {
        if (m_pIsCheckedProperty)
        {
            const auto nState = Button_GetCheck(m_hWnd);
            SetValue(*m_pIsCheckedProperty, (nState == BST_CHECKED));
            OnValueChanged();
        }
    }

protected:
    void OnViewModelBoolValueChanged(const BoolModelProperty::ChangeArgs& args) override
    {
        if (m_pIsCheckedProperty && *m_pIsCheckedProperty == args.Property)
        {
            InvokeOnUIThread([this, nValue = args.tNewValue ? BST_CHECKED : BST_UNCHECKED]() noexcept {
                Button_SetCheck(m_hWnd, nValue);
            });
            OnValueChanged();
        }
    }

    void OnValueChanged() override
    {
        ControlBinding::ForceRepaint(m_hWnd);
        ControlBinding::OnValueChanged();
    }

private:
    const BoolModelProperty* m_pIsCheckedProperty = nullptr;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_CHECKBOXBINDING_H
