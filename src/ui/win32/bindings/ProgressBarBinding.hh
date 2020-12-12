#ifndef RA_UI_WIN32_PROGRESSBARBINDING_H
#define RA_UI_WIN32_PROGRESSBARBINDING_H
#pragma once

#include "ControlBinding.hh"

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class ProgressBarBinding : public ControlBinding
{
public:
    explicit ProgressBarBinding(ViewModelBase& vmViewModel) noexcept : ControlBinding(vmViewModel) {}

    void SetHWND(DialogBase& pDialog, HWND hControl) override
    {
        ControlBinding::SetHWND(pDialog, hControl);

        if (m_pProgressProperty)
            ::SendMessage(hControl, PBM_SETPOS, GetValue(*m_pProgressProperty), 0);
    }

    void BindProgress(const IntModelProperty& pSourceProperty)
    {
        m_pProgressProperty = &pSourceProperty;

        if (m_hWnd)
            ::SendMessage(m_hWnd, PBM_SETPOS, GetValue(*m_pProgressProperty), 0);
    }

protected:
    void OnViewModelIntValueChanged(const IntModelProperty::ChangeArgs& args) override
    {
        if (m_pProgressProperty && *m_pProgressProperty == args.Property)
        {
            ::SendMessage(m_hWnd, PBM_SETPOS, GetValue(*m_pProgressProperty), 0);
            OnValueChanged();
        }
    }

private:
    const IntModelProperty* m_pProgressProperty = nullptr;
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_PROGRESSBARBINDING_H
