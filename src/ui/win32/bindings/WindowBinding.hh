#ifndef RA_UI_WIN32_WINDOWBINDING_H
#define RA_UI_WIN32_WINDOWBINDING_H
#pragma once

#include "RA_Defs.h" // windows types

#include "ui/BindingBase.hh"
#include "ui/WindowViewModelBase.hh"

#include <unordered_map>

namespace ra {
namespace ui {
namespace win32 {
namespace bindings {

class WindowBinding : protected BindingBase
{
public:
    WindowBinding(WindowViewModelBase& vmWindowViewModel) noexcept
        : BindingBase(vmWindowViewModel)
    {
    }

    void SetHWND(HWND hWnd) 
    { 
        m_hWnd = hWnd; 
        if (m_hWnd)
        {
            // immediately push values from the viewmodel to the UI
            SetWindowTextW(m_hWnd, GetValue(WindowViewModelBase::WindowTitleProperty).c_str());

            for (auto& pIter : m_mLabelBindings)
            {
                auto* pProperty = dynamic_cast<const StringModelProperty*>(ModelPropertyBase::GetPropertyForKey(pIter.first));
                if (pProperty != nullptr)
                    SetDlgItemTextW(m_hWnd, pIter.second, GetValue(*pProperty).c_str());
            }
        }
    }
    
    /// <summary>
    /// Binds the a label to a property of the viewmodel.
    /// </summary>
    /// <param name="nDlgItemId">The unique identifier of the label in the dialog.</param>
    /// <param name="pSourceProperty">The property to bind to.</param>
    void BindLabel(int nDlgItemId, const StringModelProperty& pSourceProperty)
    {
        m_mLabelBindings.insert_or_assign(pSourceProperty.GetKey(), nDlgItemId);

        if (m_hWnd)
        {
            // immediately push values from the viewmodel to the UI
            SetDlgItemTextW(m_hWnd, nDlgItemId, GetValue(pSourceProperty).c_str());
        }
    }

protected:
    void OnViewModelStringValueChanged(const StringModelProperty::ChangeArgs& args) override
    {
        if (args.Property == WindowViewModelBase::WindowTitleProperty)
        {
            SetWindowTextW(m_hWnd, args.tNewValue.c_str());
            return;
        }
        
        auto pIter = m_mLabelBindings.find(args.Property.GetKey());
        if (pIter != m_mLabelBindings.end())
        {
            SetDlgItemTextW(m_hWnd, pIter->second, args.tNewValue.c_str());
            return;
        }
    }

private:
    std::unordered_map<int, int> m_mLabelBindings;
    HWND m_hWnd{};
};

} // namespace bindings
} // namespace win32
} // namespace ui
} // namespace ra

#endif // !RA_UI_WIN32_WINDOWBINDING_H
